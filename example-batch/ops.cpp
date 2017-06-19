#include "ops.h"

//==========
// fpga/cpu
//==========

unsigned long long run_convert (const bool use_fpga,
                  const bfs::path &in_file,
                  const bfs::path &out_file,
                  const vector<string> &input) {

    const unsigned int resize_width  = boost::lexical_cast <unsigned int> (input[2]);
    const unsigned int resize_height = boost::lexical_cast <unsigned int> (input[3]);

    const unsigned int crop_width  = boost::lexical_cast <unsigned int> (input[4]);
    const unsigned int crop_height = boost::lexical_cast <unsigned int> (input[5]);

    unsigned int out_quality = 75;
    //start time
    bpt::ptime start = bpt::microsec_clock::local_time();

    if (use_fpga) {
        imaccel_convert (in_file.string(),
                         out_file.string(),
                         resize_width, resize_height,
                         0, 0,
                         crop_width, crop_height,
                         out_quality);
    }
    else {
        Mat out_img;

        // open
        out_img = imread (in_file.string());

        // resize
        resize (out_img, out_img, Size (resize_width, resize_height), 0, 0, INTER_LANCZOS4);

        // crop: left up corner
        out_img = out_img (Rect (0, 0, crop_width, crop_height));

        // save
        vector<int> compression_params;
        compression_params.push_back (IMWRITE_JPEG_QUALITY);
        compression_params.push_back (out_quality);

        imaccel_write (out_file.string() + ".jpeg", out_img, use_fpga, compression_params);
    }
    //return elapsed time in nanoseconds
    bpt::time_duration latency = bpt::microsec_clock::local_time() - start;
    return latency.total_nanoseconds();
    
}



unsigned long long run_jpeg_open (const bool use_fpga,
                    const bfs::path &in_file,
                    Mat& out_img,
                    const vector<string> &input) {

    // extract target w&h
    unsigned int resize_width, resize_height;
    std::string::size_type pos = input[2].find ('%');

    if (pos != std::string::npos) {
        Size size = imsize (in_file.string());
        double ratio  = (double) boost::lexical_cast <unsigned int> (input[2].substr (0, pos)) / 100;

        if (size.width == 0 || size.height == 0) {
            THROW_ERROR (-2, "Extracted width or height == 0: " + in_file.string());
        }

        resize_width  = size.width * ratio;
        resize_height = size.height * ratio;
    }
    else {
        resize_width  = boost::lexical_cast <unsigned int> (input[2]);
        resize_height = boost::lexical_cast <unsigned int> (input[3]);
    }

    //start time
    bpt::ptime start = bpt::microsec_clock::local_time();
    // open + resize
    if (use_fpga) {
        out_img = imread (in_file.string(), resize_width, resize_height);
    }
    else {
        out_img = imread (in_file.string());
        resize (out_img, out_img, Size (resize_width, resize_height), 0, 0, INTER_LANCZOS4);
    }

    //return elapsed time in nanoseconds
    bpt::time_duration latency = bpt::microsec_clock::local_time() - start;
    return latency.total_nanoseconds();
		   
}


unsigned long long run_jpeg_decode (const bool use_fpga,
                      const bfs::path &in_file,
                      Mat& out_img,
                      const vector<string> &input) {

    // extract target w&h
    unsigned int resize_width, resize_height;
    std::string::size_type pos = input[2].find ('%');

    if (pos != std::string::npos) {
        Size size = imsize (in_file.string());
        double ratio  = (double) boost::lexical_cast <unsigned int> (input[2].substr (0, pos)) / 100;

        if (size.width == 0 || size.height == 0) {
            THROW_ERROR (-2, "Extracted width or height == 0: " + in_file.string());
        }

        resize_width  = size.width * ratio;
        resize_height = size.height * ratio;
    }
    else {
        resize_width  = boost::lexical_cast <unsigned int> (input[2]);
        resize_height = boost::lexical_cast <unsigned int> (input[3]);
    }


    // read file into memory
    std::ifstream file (in_file.string().c_str(), std::ios::binary);

    std::vector<uchar> buff;
    buff = std::vector<uchar> ((std::istreambuf_iterator<char> (file)),
                               std::istreambuf_iterator<char>());
    //start time (start after file read)
    bpt::ptime start = bpt::microsec_clock::local_time();

    // decode
    if (use_fpga) {
        out_img = imdecode (buff, resize_width, resize_height, IMREAD_COLOR);
    }
    else {
        out_img = imdecode (buff, IMREAD_COLOR);
        resize (out_img, out_img, Size (resize_width, resize_height), 0, 0, INTER_LANCZOS4);
    }
    //return elapsed time in nanoseconds
    bpt::time_duration latency = bpt::microsec_clock::local_time() - start;
    return latency.total_nanoseconds();

}

unsigned long long run_jpeg_crop (const bool use_fpga,
                    const bfs::path &in_file,
                    Mat& out_img,
                    const vector<string> &input) {

    // extract target w&h
    Size size = imsize (in_file.string());

    if (size.width == 0 || size.height == 0) {
        THROW_ERROR (-2, "Extracted width or height == 0: " + in_file.string());
    }

    unsigned int target_width  = boost::lexical_cast <unsigned int> (input[2]);
    unsigned int target_height = boost::lexical_cast <unsigned int> (input[3]);

    double w_ratio = (double) target_width / size.width;
    double h_ratio = (double) target_height / size.height;

    double ratio = (size.width * h_ratio > target_width) ? h_ratio : w_ratio;
    unsigned int resize_width = size.width * ratio + 0.5;
    unsigned int resize_height = size.height * ratio + 0.5;
    //start time
    bpt::ptime start = bpt::microsec_clock::local_time();

    // open + resize
    if (use_fpga) {
        out_img = imread (in_file.string(), resize_width, resize_height);
    }
    else {
        out_img = imread (in_file.string());
        resize (out_img, out_img, Size (resize_width, resize_height), 0, 0, INTER_LANCZOS4);
    }

    // crop to target w&h // center
    out_img = out_img (Rect ((out_img.cols - target_width) / 2,
                             (out_img.rows - target_height) / 2,
                             target_width, target_height)); // Rect(x, y, w, h);

    //return elapsed time in nanoseconds
    bpt::time_duration latency = bpt::microsec_clock::local_time() - start;
    return latency.total_nanoseconds();

}


unsigned long long run_webp_save (const bool use_fpga,
                    Mat& out_img,
                    const bfs::path &out_file,
                    const vector<string> &input) {

    unsigned int out_quality = 75;

    if (input.size() > 2) {
        out_quality = boost::lexical_cast <unsigned int> (input[2]);
    }

    vector<int> compression_params;
    compression_params.push_back (IMWRITE_WEBP_QUALITY);
    compression_params.push_back (out_quality);
    //start time
    bpt::ptime start = bpt::microsec_clock::local_time();
    imaccel_write (out_file.string() + ".webp", out_img, use_fpga, compression_params);
    //return elapsed time in nanoseconds
    bpt::time_duration latency = bpt::microsec_clock::local_time() - start;
    return latency.total_nanoseconds();

}



unsigned long long run_webp_encode (const bool use_fpga,
                      const bool force_output,
                      Mat& out_img,
                      const bfs::path &out_file,
                      const vector<string> &input) {

    unsigned int out_quality = 75;

    if (input.size() > 2) {
        out_quality = boost::lexical_cast <unsigned int> (input[2]);
    }

    vector<int> compression_params;
    compression_params.push_back (IMWRITE_WEBP_QUALITY);
    compression_params.push_back (out_quality);

    vector<uchar> buf;
    //start time
    bpt::ptime start = bpt::microsec_clock::local_time();

    imaccel_encode (".webp", out_img, buf, use_fpga, compression_params);

    //return elapsed time in nanoseconds
    bpt::time_duration latency = bpt::microsec_clock::local_time() - start;
    return latency.total_nanoseconds();

    if (force_output) {
        //write file for confirmation
        ofstream f_output ((out_file.string() + ".webp").c_str(), ofstream::binary);
        f_output.write ((char *)&buf[0], buf.size()*sizeof (uchar));
        f_output.close();
    }
}



//==========
// pure cpu
//==========

unsigned long long run_open (const bool use_fpga,
               const bfs::path &in_file,
               Mat& out_img,
               const vector<string> &input) {
    //start time
    bpt::ptime start = bpt::microsec_clock::local_time();
    out_img = imread (in_file.string());
    //return elapsed time in nanoseconds
    bpt::time_duration latency = bpt::microsec_clock::local_time() - start;
    return latency.total_nanoseconds();
}



unsigned long long run_decode (const bool use_fpga,
                 const bfs::path &in_file,
                 Mat& out_img,
                 const vector<string> &input) {
    // read file into memory
    std::ifstream file (in_file.string().c_str(), std::ios::binary);

    std::vector<uchar> buff;
    buff = std::vector<uchar> ((std::istreambuf_iterator<char> (file)),
                               std::istreambuf_iterator<char>());

    //start time (start after file read)
    bpt::ptime start = bpt::microsec_clock::local_time();

    // decode
    out_img = imdecode (buff, IMREAD_COLOR);
    //return elapsed time in nanoseconds
    bpt::time_duration latency = bpt::microsec_clock::local_time() - start;
    return latency.total_nanoseconds();

}



unsigned long long run_resize (const bool use_fpga,
                 Mat& in_img,
                 Mat& out_img,
                 const vector<string> &input) {

    //start time
    bpt::ptime start = bpt::microsec_clock::local_time();
    // extract target w&h
    unsigned int resize_width, resize_height;
    std::string::size_type pos = input[2].find ('%');

    if (pos != std::string::npos) {
        double ratio  = (double) boost::lexical_cast <unsigned int> (input[2].substr (0, pos)) / 100;

        resize_width  = in_img.cols * ratio;
        resize_height = in_img.rows * ratio;
    }
    else {
        resize_width  = boost::lexical_cast <unsigned int> (input[2]);
        resize_height = boost::lexical_cast <unsigned int> (input[3]);
    }

    // resize
    resize (in_img, out_img, Size (resize_width, resize_height), 0, 0, INTER_LANCZOS4);
    //return elapsed time in nanoseconds
    bpt::time_duration latency = bpt::microsec_clock::local_time() - start;
    return latency.total_nanoseconds();

}



unsigned long long run_crop (const bool use_fpga,
               Mat& in_img,
               Mat& out_img,
               const vector<string> &input) {
    //start time
    bpt::ptime start = bpt::microsec_clock::local_time();
    const unsigned int crop_width  = boost::lexical_cast <unsigned int> (input[2]);
    const unsigned int crop_height = boost::lexical_cast <unsigned int> (input[3]);

    // center
    out_img = in_img (Rect ((in_img.cols - crop_width) / 2,
                            (in_img.rows - crop_height) / 2,
                            crop_width, crop_height)); // Rect (x, y, w, h);
    //return elapsed time in nanoseconds
    bpt::time_duration latency = bpt::microsec_clock::local_time() - start;
    return latency.total_nanoseconds();

}



unsigned long long run_save (const bool use_fpga,
               Mat& in_img,
               const bfs::path &out_file,
               const vector<string> &input) {
    //do not time write
    string out_type = input[2];
    unsigned int out_quality = 75;

    if (input.size() > 3) {
        out_quality = boost::lexical_cast <unsigned int> (input[3]);
    }

    vector<int> compression_params;
    compression_params.push_back (out_type == "webp" ?
                                  IMWRITE_WEBP_QUALITY : IMWRITE_JPEG_QUALITY);
    compression_params.push_back (out_quality);

    imaccel_write (out_file.string() + "." + out_type, in_img, use_fpga, compression_params);

    //return elapsed time in nanoseconds
    return 0;

}


unsigned long long run_encode (const bool use_fpga,
                 const bool force_output,
                 Mat& in_img,
                 const bfs::path &out_file,
                 const vector<string> &input) {
    //start time
    bpt::ptime start = bpt::microsec_clock::local_time();
    string out_type = input[2];
    unsigned int out_quality = 75;

    if (input.size() > 3) {
        out_quality = boost::lexical_cast <unsigned int> (input[3]);
    }

    vector<int> compression_params;
    compression_params.push_back (out_type == "webp" ?
                                  IMWRITE_WEBP_QUALITY : IMWRITE_JPEG_QUALITY);
    compression_params.push_back (out_quality);

    vector<uchar> buf;
    imaccel_encode ("." + out_type, in_img, buf, use_fpga, compression_params);
    
    //do not time write
    bpt::time_duration latency = bpt::microsec_clock::local_time() - start;

    if (force_output) {
        //write file for confirmation
        ofstream f_output ((out_file.string() + out_type).c_str(), ofstream::binary);
        f_output.write ((char *)&buf[0], buf.size()*sizeof (uchar));
        f_output.close();
    }
    //return elapsed time in nanoseconds
    return latency.total_nanoseconds();

}


unsigned long long run_main_color (const bool use_fpga,
                     const bool force_output,
                     Mat& in_img,
                     const bfs::path &out_file,
                     const vector<string> &input) {
    //start time
    bpt::ptime start = bpt::microsec_clock::local_time();
    Octree tree;
    tree.buildTree (in_img, 8);
    Scalar main_color = tree.primaryColor();
    //do not time stdout
    bpt::time_duration latency = bpt::microsec_clock::local_time() - start;
    cout << "main color [bgra]: " << main_color << endl;
    //return elapsed time in nanoseconds
    return latency.total_nanoseconds();

}



unsigned long long run_watermark (const bool use_fpga,
                    const bool force_output,
                    Mat& in_img,
                    const bfs::path &out_file,
                    const vector<string> &input) {
    //start time
    bpt::ptime start = bpt::microsec_clock::local_time();
    //do not time stdout
    bpt::time_duration latency = bpt::microsec_clock::local_time() - start;
    cout << "NOT implemented: watermark" << endl;
    //return elapsed time in nanoseconds

    return latency.total_nanoseconds();

}



unsigned long long run_sharpen (Mat &in_img) {
    Mat blur_img;
    //start time
    bpt::ptime start = bpt::microsec_clock::local_time();
    GaussianBlur (in_img, blur_img, Size (0, 0), 3);
    addWeighted (in_img, 1.5, blur_img, -0.5, 0, in_img);
    //return elapsed time in nanoseconds
    bpt::time_duration latency = bpt::microsec_clock::local_time() - start;
    return latency.total_nanoseconds();

}



//=========
// others
//=========

// octree algorithm modifeid from:
// https://github.com/XadillaX/thmclrx/blob/master/src/octree.cpp

Octree::Octree() {
    root = new OctreeNode();
    leafCount = 0;
}

Octree::~Octree() {
    delete (root);
}

void Octree::buildTree (Mat& m, int maxColor) {
    int ch = m.channels();
    unsigned char alpha, b, g, r;
    for (int y = 0; y < m.rows; y++) {
        for (int x = 0; x < m.cols; x++) {
            if (ch == 4) {
                alpha = m.data[y * m.step + x * m.channels() + 3];
            }
            else {
                alpha = 255;
            }
            // if alpha is 0, skip this pixels
            if (alpha == 0) {
                continue;
            }

            b = m.data[y * m.step + x * m.channels() + 0];
            g = m.data[y * m.step + x * m.channels() + 1];
            r = m.data[y * m.step + x * m.channels() + 2];

            addColor (root, r, g, b, 0);

            while (leafCount > maxColor) {
                reduceTree();
            }
            totalPixels++;
        }
    }
}



void Octree::addColor (OctreeNode* node, unsigned char r, unsigned char g,
                       unsigned char b, int level) {
    if (node->isLeaf) {
        node->pixelCount++;
        node->redSum += r;
        node->greenSum += g;
        node->blueSum += b;
    }
    else {
        unsigned char rBit = (r >> (7 - level)) & 1;
        unsigned char gBit = (g >> (7 - level)) & 1;
        unsigned char bBit = (b >> (7 - level)) & 1;

        int idx = (rBit << 2) + (gBit << 1) + bBit;

        if (node->children[idx] == NULL) {
            OctreeNode* tmp = new OctreeNode();
            node->children[idx] = tmp;
            if (level == 7) {
                tmp->isLeaf = true;
                leafCount++;
            }
            else {
                reducible[level].push_front (tmp);
            }
        }

        addColor (node->children[idx], r, g, b, level + 1);
    }
}



void Octree::reduceTree() {
    // find the deepest level of node
    int lv = 6;
    while (reducible[lv].empty() && lv >= 0) {
        lv--;
    }

    if (lv < 0) {
        return;
    }

    // get the node and remove it from reducible link
    OctreeNode* node = reducible[lv].front();
    reducible[lv].pop_front();

    // merge children
    int r = 0;
    int g = 0;
    int b = 0;
    int count = 0;
    for (int i = 0; i < 8; i++) {
        if (node->children[i] == NULL) {
            continue;
        }

        r += node->children[i]->redSum;
        g += node->children[i]->greenSum;
        b += node->children[i]->blueSum;
        count += node->children[i]->pixelCount;
        leafCount--;

        delete node->children[i];
        node->children[i] = NULL;
    }

    node->isLeaf = true;
    node->redSum = r;
    node->greenSum = g;
    node->blueSum = b;
    node->pixelCount = count;
    leafCount++;
}



void Octree::colorStats (OctreeNode* node, vector<ColorCounter>& colorCounters) {
    if (node->isLeaf) {
        int r = node->redSum / node->pixelCount;
        int g = node->greenSum / node->pixelCount;
        int b = node->blueSum / node->pixelCount;
        ColorCounter cc = ColorCounter();
        cc.red = r;
        cc.green = g;
        cc.blue = b;
        cc.num = node->pixelCount;
        colorCounters.push_back (cc);
        return;
    }

    for (int i = 0; i < 8; i++) {
        if (node->children[i] != NULL) {
            colorStats (node->children[i], colorCounters);
        }
    }
}



Scalar Octree::primaryColor() {
    vector<ColorCounter> v;
    colorStats (root, v);
    if (v.size() == 0) {
        return Scalar (255, 255, 255);
    }

    Scalar color;
    int max = 0;
    vector<ColorCounter>::iterator begin = v.begin();
    vector<ColorCounter>::iterator end = v.end();
    for (vector<ColorCounter>::iterator it = begin; it != end; ++it) {
        if (it->num > max) {
            max = it->num;
            color = Scalar (it->blue, it->green, it->red);
        }
    }
    return color;
}

