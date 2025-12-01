#include <iostream>
#include <zmq.h>
#include <vector>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>
#include <cerrno>           



int main(){

    void* context = zmq_ctx_new();
    void* pull_socket = zmq_socket(context, ZMQ_PULL);
    void* push_socket = zmq_socket(context, ZMQ_PUSH);

    int rc_pull = zmq_connect(pull_socket, "tcp://localhost:5555");
    int rc_push = zmq_bind(push_socket, "tcp://*:5556");

    if(rc_pull != 0){
        std::cerr << "Faild to connect to the ZMQ PULL socket: " << zmq_strerror(errno) << "\n";
        return 0; 
    }

    std::cout << "Connected the ZMQ PULL socket. " << "\n";

    if(rc_push != 0){
      std::cerr << "Faild to connect to the ZMQ PUSH socket:" << zmq_strerror(errno) <<"\n";
      return 0;
    }
    std::cout << "ZMQ push socket bound on tcp://*:5556" << "\n";

    auto sift = cv::SIFT::create();
    while(true){
        zmq_msg_t metadata_msg;
        zmq_msg_init (&metadata_msg);
        int rc_meta = zmq_msg_recv(&metadata_msg, pull_socket, 0);
        if(rc_meta == -1){
            std::cerr << "[ERROR] zmq_msg_recv(meta) failed: "
                  << zmq_strerror(errno) << "\n";
            zmq_msg_close(&metadata_msg);
            continue;
        }
        std::string metadata_str(
            static_cast<char*> (zmq_msg_data(&metadata_msg)), 
            zmq_msg_size(&metadata_msg));
        zmq_msg_close(&metadata_msg);
        nlohmann::json metadata;
        try{
            metadata = nlohmann::json::parse(metadata_str);
        } catch (const std::exception& e){
            std::cerr<< "[ERROR] Failed to parse metadata JSON: " << e.what() << "\n";
            continue;
        }
        std::cout << "Received meta: " << metadata.dump() << "\n";
        

        zmq_msg_t img_msg;
        zmq_msg_init(&img_msg);
        int rc_img = zmq_msg_recv(&img_msg, pull_socket, 0);
        if (rc_img == -1){
            std::cerr << "[ERROR] zmq_msg_recv(image) failed: " << zmq_strerror(errno) << "\n";
            zmq_msg_close(&img_msg);
            continue;
        }

        std::vector<uchar> buf (
            static_cast<uchar*>(zmq_msg_data(&img_msg)),
            static_cast<uchar*>(zmq_msg_data(&img_msg)) + zmq_msg_size(&img_msg)
        );
        std::cout<< buf.size()<<"\n";
        zmq_msg_close(&img_msg);
        std::cout << "Received image buffer size: " << buf.size() << "\n";

        cv::Mat img = cv::imdecode(buf, cv::IMREAD_COLOR);
        if(img.empty()){
            std::cerr << "[ERROR] Failed to decode received image." <<"\n";
            continue;
        }
        std::cout << "Decoded image: " << img.cols << "x" << img.rows << "\n";
        std::vector <cv::KeyPoint> keypoints;
        cv::Mat desc;

        sift->detectAndCompute(img, cv::noArray(), keypoints, desc);

        std::cout << "Extracted " << keypoints.size()
                    << " keypoints for seq="
                    << metadata.value("seq_number", -1)
                    << "\n";

        nlohmann::json feature_data = metadata;
        feature_data["keypoint_count"] = keypoints.size();

        nlohmann::json kp_array = nlohmann::json::array();
        for(const auto& kp: keypoints){
            kp_array.push_back({
                {"x", kp.pt.x},
                {"y", kp.pt.y}, 
                {"size", kp.size},
                {"angle", kp.angle }, 
                {"response", kp.response}, 
                {"octave", kp.octave}
            });
        }
        feature_data["keypoints"] = kp_array;

        std::string feature_str = feature_data.dump();
        int rc_feature = zmq_send(
            push_socket, 
            feature_str.data(),
            feature_str.size(), 
            ZMQ_SNDMORE | ZMQ_DONTWAIT 
        );

        if(rc_feature == -1){
            if(errno == EAGAIN){
                std::cerr << "[WARN] No downstream logger (feature), dropping frame " 
                << feature_data.value("seq_number", -1) << "\n";
            }
            else{
                std::cerr << "[ERROR] zmq_send(feature to data_logger) failed: " 
                << zmq_strerror(errno) << "\n";
            }
            continue;
        }

        int rc_img_push = zmq_send(
            push_socket, 
            buf.data(), 
            buf.size(), 
            ZMQ_DONTWAIT
        );

        if(rc_img_push == -1){
            if (errno == EAGAIN){
                std::cerr << "[WARN] No downstream logger (image), dropping frame  " 
                << feature_data.value("seq_number", -1) << "\n";
            }
            else{
                std::cerr << "[ERROR] zmq_send(image to data_logger) failed: "
                << zmq_strerror(errno) << "\n";

            }
            continue;
        }

        std::cout << "Forwarded seq="
        << feature_data.value("seq_number", -1) 
        << "with " << keypoints.size()
        << "keypoints to data logger app\n";
    }
    zmq_close(pull_socket);
    zmq_close(push_socket);
    zmq_ctx_term(context);
    return 0; 

}