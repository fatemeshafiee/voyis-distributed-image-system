#include <iostream>
#include <opencv2/imgcodecs.hpp> 
#include <opencv2/core.hpp>
#include <sqlite3.h>
#include <zmq.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <vector>
#include <algorithm>
namespace fs = std::filesystem;
int main(int argc, char** argv){
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
    std::cerr << "No image (.png/.jpg/.jpeg/.bmp) found in the: " <<folder_address <<"\n";
    return 1;
  }
  std::cout << "Found " << image_files.size() << " image(s) in " << folder_address << "\n";


  std::size_t seq_number = 0;
  for(const auto&image_path : image_files ){
  
  cv::Mat img = cv::imread(image_path.string(), cv::IMREAD_COLOR);
  if(img.empty()){
    std::cerr << "the image is empty" << "\n" ;
  }
  else{
    std::cout << "the image clos " << img.cols << " and the rows " << img.rows << "\n";
  }
  std::vector<uchar> buf;
  if(!cv::imencode(".png", img , buf)){
    std::cerr<<"failed to encode image to png." << "\n";
  }else{
    std::cout << "the buffer size is "<< buf.size() <<"\n";
  }
  nlohmann::json metadata;
  metadata["seq_number"] = seq_number;
  metadata["image_name"] = image_path.filename().string();
  metadata["rows"] = img.rows;
  metadata["cols"] = img.cols;
  metadata["encoding"]   = "png";           
  metadata["data_bytes"] = buf.size();   
  metadata["encoding"] = buf;
  // std::cout << metadata.dump() << "\n";
  seq_number++;

  }



  return 0;
}