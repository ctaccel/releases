#include <iostream>
#include <stdexcept>
#include <sstream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#include <boost/filesystem.hpp>

using namespace std;
using namespace cv;
using namespace boost::filesystem;

int main (int argc, char *argv[]) {

    if (argc != 3) {
        stringstream ss;
        ss << "Usage: " << argv[0] << " input_file output_file" << endl;
        throw runtime_error (ss.str ());
    }

    cv::setNumThreads (1);

    path in_path (argv[1]),
         out_path (argv[2]);

    if (is_regular_file (in_path)) {

        //=========================
        // open as a 300x300 image
        //=========================

        Mat in_img = imread (in_path.string (), 300, 300);

        //======================
        // save with quality=75
        //======================

        vector<int> compression_params;
        compression_params.push_back (IMWRITE_WEBP_QUALITY);
        compression_params.push_back (75);

        imwrite (out_path.string (), in_img, compression_params);

        cout << in_path << " saved as " << out_path << endl;
    }
    else {
        stringstream ss;
        ss << in_path.string () << " is not a regular file" << endl;
        throw runtime_error (ss.str ());
    }

    return 0;
}

