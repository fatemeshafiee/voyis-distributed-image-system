// feature_extractor: receives images from tcp://localhost:5555 (PULL),
// runs SIFT, adds keypoint metadata, and forwards to tcp://*:5556.
#include <iostream>
#include <zmq.h>
#include <vector>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>
#include <cerrno>           
#include "common/frame.hpp"
#include "common/zmq_utils.hpp"


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
        auto metadata_str_opt = zmq_utils::recv_string(
            pull_socket,
            0,
            "zmq_msg_recv(meta)"
        );
        if (!metadata_str_opt) {
            continue;
        }
        auto meta_opt = FrameMetadata::from_json(*metadata_str_opt);
        if (!meta_opt) {
            std::cerr << "[ERROR] Failed to parse metadata JSON\n";
            continue;
        }
        FrameMetadata meta = *meta_opt;
        std::cout << "Received meta: " << meta.to_json().dump() << "\n";

        

        auto buf_opt = zmq_utils::recv_bytes(
            pull_socket,
            0,
            "zmq_msg_recv(image)"
        );
        if (!buf_opt) {
            continue;
        }
        std::vector<unsigned char> buf = std::move(*buf_opt);
        std::cout<< buf.size()<<"\n";
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
                    << meta.seq_number
                    << "\n";

        FrameMetadata out_meta = meta;
        out_meta.keypoint_count = static_cast<int>(keypoints.size());

        nlohmann::json feature_data = out_meta.to_json();

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
        auto feature_rc = zmq_utils::send_string(
            push_socket,
            feature_str,
            ZMQ_SNDMORE | ZMQ_DONTWAIT,
            "zmq_send(feature to data_logger)"
        );

        if(feature_rc == zmq_utils::SendResult::WouldBlock){
            std::cerr << "[WARN] No downstream logger (feature), dropping frame " 
            << out_meta.seq_number  << "\n";
            continue;
        }
        if(feature_rc == zmq_utils::SendResult::Error){
            continue;
        }

        auto img_rc = zmq_utils::send_bytes(
            push_socket,
            buf,
            ZMQ_DONTWAIT,
            "zmq_send(image to data_logger)"
        );

        if(img_rc == zmq_utils::SendResult::WouldBlock){
            std::cerr << "[WARN] No downstream logger (image), dropping frame  " 
            << out_meta.seq_number << "\n";
            continue;
        }
        if(img_rc == zmq_utils::SendResult::Error){
            continue;
        }

        std::cout << "Forwarded seq="
        << out_meta.seq_number
        << "with " << keypoints.size()
        << "keypoints to data logger app\n";
    }
    zmq_close(pull_socket);
    zmq_close(push_socket);
    zmq_ctx_term(context);
    return 0; 

}
