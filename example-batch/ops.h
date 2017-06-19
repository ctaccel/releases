#pragma once

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>

#define THROW_ERROR(err_code, err_msg)                 \
    {                                                  \
        ostringstream oss;                             \
        oss << err_msg << ", err_code = " << err_code; \
        throw runtime_error (oss.str ());              \
    }

using namespace std;
using namespace cv;

namespace bfs = boost::filesystem;
namespace ba  = boost::algorithm;
namespace bpt = boost::posix_time;



//==========
// fpga/cpu
//==========

unsigned long long run_convert (const bool use_fpga,
                  const bfs::path &in_file,
                  const bfs::path &out_file,
                  const vector<string> &input);
unsigned long long run_jpeg_open (const bool use_fpga,
                    const bfs::path &in_file,
                    Mat& out_img,
                    const vector<string> &input);

unsigned long long run_jpeg_decode (const bool use_fpga,
                      const bfs::path &in_file,
                      Mat& out_img,
                      const vector<string> &input);

unsigned long long run_jpeg_crop (const bool use_fpga,
                    const bfs::path &in_file,
                    Mat& out_img,
                    const vector<string> &input);

unsigned long long run_webp_save (const bool use_fpga,
                    Mat& out_img,
                    const bfs::path &out_file,
                    const vector<string> &input);

unsigned long long run_webp_encode (const bool use_fpga,
                      const bool force_output,
                      Mat& out_img,
                      const bfs::path &out_file,
                      const vector<string> &input);

//==========
// pure cpu
//==========

unsigned long long run_open (const bool use_fpga,
               const bfs::path &in_file,
               Mat& out_img,
               const vector<string> &input);

unsigned long long run_decode (const bool use_fpga,
                 const bfs::path &in_file,
                 Mat& out_img,
                 const vector<string> &input);

unsigned long long run_resize (const bool use_fpga,
                 Mat& in_img,
                 Mat& out_img,
                 const vector<string> &input);

unsigned long long run_crop (const bool use_fpga,
               Mat& in_img,
               Mat& out_img,
               const vector<string> &input);

unsigned long long run_save (const bool use_fpga,
               Mat& in_img,
               const bfs::path &out_file,
               const vector<string> &input);

unsigned long long run_encode (const bool use_fpga,
                 const bool force_output,
                 Mat& in_img,
                 const bfs::path &out_file,
                 const vector<string> &input);

unsigned long long run_main_color (const bool use_fpga,
                     const bool force_output,
                     Mat& in_img,
                     const bfs::path &out_file,
                     const vector<string> &input);

unsigned long long run_watermark (const bool use_fpga,
                    const bool force_output,
                    Mat& in_img,
                    const bfs::path &out_file,
                    const vector<string> &input);

unsigned long long run_sharpen (Mat &in_img);



//=========
// others
//=========

// octree algorithm modifeid from:
// https://github.com/XadillaX/thmclrx/blob/master/src/octree.cpp

class OctreeNode {
public:
    ~OctreeNode() {
        for (int i = 0; i < 8; ++i) {
            delete children[i];
        }
    };
    int redSum;
    int greenSum;
    int blueSum;

    bool isLeaf;
    int pixelCount;
    OctreeNode* children[8];
};


class ColorCounter {
public:
    int red;
    int green;
    int blue;
    int num;
};


class Octree {
private:
    OctreeNode* root;
    list<OctreeNode*> reducible[7];
    int leafCount;
    int totalPixels;
    void freeTree (OctreeNode* root);

public:
    Octree();
    ~Octree();
    void buildTree (Mat& m, int maxColor);
    void addColor (OctreeNode* node, unsigned char r, unsigned char g,
                   unsigned char b, int level);
    void reduceTree();
    void colorStats (OctreeNode* node, vector<ColorCounter>& colorCounters);
    Scalar primaryColor();
    OctreeNode* getRoot() {
        return root;
    }
};

