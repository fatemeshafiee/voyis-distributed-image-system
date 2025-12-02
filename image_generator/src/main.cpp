// image_generator/main.cpp
// App 1: Reads images from a folder, encodes them as PNG, and streams
// (metadata JSON + binary image) over ZeroMQ PUSH on ipc:///tmp/voyis-image-stream.ipc.

#include <iostream>
#include <opencv2/imgcodecs.hpp> 
#include <opencv2/core.hpp>
#include <sqlite3.h>
#include <zmq.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <system_error>
#include <vector>
#include <algorithm>
#include <thread> 
#include <chrono>
#include <csignal>
#include <cerrno>
#include "common/frame.hpp"
#include "common/zmq_utils.hpp"

bool running = true;

// void signal_handler(int) {
//     running = false;
// }
namespace fs = std::filesystem;

namespace {
constexpr char kImageStreamEndpoint[] = "ipc:///tmp/voyis-image-stream.ipc";
constexpr char kImageStreamPath[] = "/tmp/voyis-image-stream.ipc";
}
int main(int argc, char** argv){
  // std::signal(SIGINT, signal_handler);

  if(argc < 2){
    std::cerr << "Usage: " << argv[0] << " <image_folder>\n";
    return 1;
  }
  std::string folder_address = argv[1];
  fs::path folder_path(folder_address);
  if(!fs::exists(folder_path) || !fs::is_directory(folder_path)){
    std::cerr << "Error: the folder " << folder_address << "address is incorrect. \n";
    return 1;   
  }
  std::vector<fs::path> image_files;
  for (const auto &entry: fs::directory_iterator(folder_path)){
    if (!entry.is_regular_file()) continue;
    auto ext = entry.path().extension().string();
//     explain
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp"){
      image_files.push_back(entry.path());
    }
  }
  if(image_files.empty()){
    std::cerr << "No image (.png/.jpg/.jpeg/.bmp) found in the: " 
      <<folder_address <<"\n";
    return 1;
  }
  std::cout << "Found " << image_files.size() 
      << " image(s) in " << folder_address << "\n";

  void* context = zmq_ctx_new();
  void* socket = zmq_socket(context, ZMQ_PUSH);
  std::error_code remove_ec;
  fs::remove(kImageStreamPath, remove_ec);
  int rc = zmq_bind(socket, kImageStreamEndpoint);
  if(rc!=0){
    std::cerr<< "failed to bind ZMQ socket: " << zmq_strerror(errno) <<"\n";
    return 1;
  }
  std::cout<<"ZMQ push socket bound on " << kImageStreamEndpoint <<"\n"; 
  std::size_t seq_number = 0;
  while(true){
    
    for(const auto&image_path : image_files ){
    
      cv::Mat img = cv::imread(image_path.string(), cv::IMREAD_COLOR);
      if(img.empty())
        std::cerr << "the image is empty" << "\n" ;
      else
        std::cout << "Image cols " << img.cols 
            << ", rows " << img.rows << "\n";
      
      std::vector<uchar> buf;
      if(!cv::imencode(".png", img , buf))
        std::cerr<<"failed to encode image to png." << "\n";
      else
        std::cout << "the buffer size is "<< buf.size() <<"\n";
      
      FrameMetadata meta;
      meta.seq_number  = seq_number;
      meta.image_name = image_path.filename().string();
      meta.rows  = img.rows;
      meta.cols = img.cols;
      meta.encoding   = "png";           
      meta.keypoint_count = buf.size();   

      std::string metadata_str = meta.to_json().dump();
      std::cout<< "trying to send data" <<"\n";
      auto meta_rc = zmq_utils::send_string(
          socket,
          metadata_str,
          ZMQ_SNDMORE | ZMQ_DONTWAIT,
          "zmq_send(meta)"
      );
      if(meta_rc == zmq_utils::SendResult::WouldBlock){
        std::cerr << "[WARN] No downstream receiver (meta), dropping frame "
                  << seq_number << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        continue;
      }
      if(meta_rc == zmq_utils::SendResult::Error){
        continue;
      }

      auto img_rc = zmq_utils::send_bytes(
          socket,
          buf,
          ZMQ_DONTWAIT,
          "zmq_send(image)"
      );
      if (img_rc == zmq_utils::SendResult::WouldBlock){
        std::cerr << "[WARN] No downstream receiver (image), dropping frame "
                  << seq_number << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        continue;
      }
      if (img_rc == zmq_utils::SendResult::Error){
        continue;
      }
      std::cout << "Sent frame seq=" << seq_number 
          << " bytes=" << buf.size() << "\n";


      seq_number++;

      std::this_thread::sleep_for(std::chrono::milliseconds(500));

    }
  }
  std::cout<< "Closing the socket and terminating the context." <<"\n";
  zmq_close(socket);
  zmq_ctx_term(context);


  return 0;
}
