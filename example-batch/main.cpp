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
// pure cpu
//==========

void run_open (const bool use_fpga,
               const bfs::path &in_file,
               Mat& out_img,
               const vector<string> &input) {

    out_img = imread (in_file.string());
}



void run_decode (const bool use_fpga,
                 const bfs::path &in_file,
                 Mat& out_img,
                 const vector<string> &input) {

    // read file into memory
    std::ifstream file (in_file.string().c_str(), std::ios::binary);

    std::vector<uchar> buff;
    buff = std::vector<uchar> ((std::istreambuf_iterator<char> (file)),
                               std::istreambuf_iterator<char> ());

    // decode
    out_img = imdecode (buff, IMREAD_COLOR);
}



void run_resize (const bool use_fpga,
                 Mat& in_img,
                 Mat& out_img,
                 const vector<string> &input) {

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
}



void run_crop (const bool use_fpga,
               Mat& in_img,
               Mat& out_img,
               const vector<string> &input) {

    const unsigned int crop_width  = boost::lexical_cast <unsigned int> (input[2]);
    const unsigned int crop_height = boost::lexical_cast <unsigned int> (input[3]);

    // center
    out_img = in_img (Rect ((in_img.cols - crop_width) / 2,
                            (in_img.rows - crop_height) / 2,
                            (in_img.cols + crop_width) / 2,
                            (in_img.rows + crop_height) / 2));
}



void run_save (const bool use_fpga,
               Mat& in_img,
               const bfs::path &out_file,
               const vector<string> &input) {

    string out_type = input[2];

    if (out_type == "bgr") {
        ofstream outfile;
        outfile.open (out_file.string() + ".bgr", ios::out | ios::binary);
        const char *bgr = (const char *) in_img.data;
        for (int r = 0; r < in_img.rows; ++r) {
            outfile.write (&bgr[r * in_img.step], 3 * in_img.cols);
        }
        outfile.close();

        //ofstream outfile;
        //outfile.open (out_file.string() + ".bgr");
        //unsigned char *bgr = (unsigned char *) in_img.data;
        //for (int r = 0; r < in_img.rows; ++r) {
        //    for (int c = 0; c < in_img.cols; ++c) {
        //        outfile << setw (3) << setfill ('0') << (int) bgr[r * in_img.step + c    ]
        //                << setw (3) << setfill ('0') << (int) bgr[r * in_img.step + c + 1]
        //                << setw (3) << setfill ('0') << (int) bgr[r * in_img.step + c + 2] << " ";
        //    }
        //    outfile << endl;
        //}
        //outfile.close();
    }
    else {
        unsigned int out_quality = 75;

        if (input.size() > 3) {
            out_quality = boost::lexical_cast <unsigned int> (input[3]);
        }

        vector<int> compression_params;
        compression_params.push_back (out_type == "webp" ?
                                      IMWRITE_WEBP_QUALITY : IMWRITE_JPEG_QUALITY);
        compression_params.push_back (out_quality);

        imwrite (out_file.string() + "." + out_type, in_img, compression_params, use_fpga);
    }
}


void run_encode (const bool use_fpga,
                 Mat& in_img,
                 const bfs::path &out_file,
                 const vector<string> &input) {

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
    imencode ("." + out_type, in_img, buf, compression_params, use_fpga);
}



//==========
// fpga/cpu
//==========

void run_convert (const bool use_fpga,
                  const bfs::path &in_file,
                  const bfs::path &out_file,
                  const vector<string> &input) {

    const unsigned int resize_width  = boost::lexical_cast <unsigned int> (input[2]);
    const unsigned int resize_height = boost::lexical_cast <unsigned int> (input[3]);

    const unsigned int crop_width  = boost::lexical_cast <unsigned int> (input[4]);
    const unsigned int crop_height = boost::lexical_cast <unsigned int> (input[5]);

    unsigned int out_quality = 75;

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

        imwrite (out_file.string() + ".jpeg", out_img, compression_params, use_fpga);
    }
}



void run_jpeg_open (const bool use_fpga,
                    const bfs::path &in_file,
                    Mat& out_img,
                    const vector<string> &input) {

    // extract target w&h
    unsigned int resize_width, resize_height;

    std::string::size_type pos = input[2].find ('%');

    if (pos != std::string::npos) {
        const Size size = imsize (in_file.string());
        const double ratio  = (double) boost::lexical_cast <unsigned int> (input[2].substr (0, pos)) / 100;
        resize_width  = size.width * ratio;
        resize_height = size.height * ratio;
    }
    else {
        resize_width  = boost::lexical_cast <unsigned int> (input[2]);
        resize_height = boost::lexical_cast <unsigned int> (input[3]);
    }

    // open + resize
    if (use_fpga) {
        out_img = imread (in_file.string(), resize_width, resize_height);
    }
    else {
        out_img = imread (in_file.string());
        resize (out_img, out_img, Size (resize_width, resize_height), 0, 0, INTER_LANCZOS4);
    }
}



void run_jpeg_decode (const bool use_fpga,
                      const bfs::path &in_file,
                      Mat& out_img,
                      const vector<string> &input) {

    // extract target w&h
    unsigned int resize_width, resize_height;

    std::string::size_type pos = input[2].find ('%');

    if (pos != std::string::npos) {
        const Size size = imsize (in_file.string());
        const double ratio  = (double) boost::lexical_cast <unsigned int> (input[2].substr (0, pos)) / 100;
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
                               std::istreambuf_iterator<char> ());

    // decode
    if (use_fpga) {
        out_img = imdecode (buff, resize_width, resize_height, IMREAD_COLOR);
    }
    else {
        out_img = imdecode (buff, IMREAD_COLOR);
        resize (out_img, out_img, Size (resize_width, resize_height), 0, 0, INTER_LANCZOS4);
    }
}



void run_webp_save (const bool use_fpga,
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

    imwrite (out_file.string() + ".webp", out_img, compression_params, use_fpga);
}



void run_webp_encode (const bool use_fpga,
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
    imencode (".webp", out_img, buf, compression_params, use_fpga);

    //write file for confirmation
}



void run_test_case (const bool non_ref,
                    const bfs::path &in_file,
                    const bfs::path &out_file,
                    vector<string> &ops) {

    Mat out_img;

    for (int i = 1; i < ops.size() - 1; i++) {

        ba::to_lower (ops[i]);

        vector<string> tokens;
        boost::split (tokens, ops[i], boost::is_any_of ("\tx "), ba::token_compress_on);

        if (tokens.size() > 0) {

            //======
            // fpga
            //======

            if (tokens[0] == "fpga") {
                const bool use_fpga = true;

                if (tokens[1] == "convert") {
                    run_convert (non_ref && use_fpga,
                                 in_file, out_file, tokens);
                }
                else if (tokens[1] == "jpeg_open") {
                    run_jpeg_open (non_ref && use_fpga,
                                   in_file, out_img, tokens);
                }
                else if (tokens[1] == "jpeg_decode") {
                    run_jpeg_decode (non_ref && use_fpga,
                                     in_file, out_img, tokens);
                }
                else if (tokens[1] == "webp_save") {
                    run_webp_save (non_ref && use_fpga,
                                   out_img, out_file, tokens);
                }
                else if (tokens[1] == "webp_encode") {
                    run_webp_encode (non_ref && use_fpga,
                                     out_img, out_file, tokens);
                }
                else {
                    THROW_ERROR (-1, "invalid operation for fpga");
                }
            }

            //======
            // cpu
            //======

            else if (tokens[0] == "cpu") {
                const bool use_fpga = false;

                // fpga flow by cpu
                if (tokens[1] == "convert") {
                    run_convert (non_ref && use_fpga,
                                 in_file, out_file, tokens);
                }
                else if (tokens[1] == "jpeg_open") {
                    run_jpeg_open (non_ref && use_fpga,
                                   in_file, out_img, tokens);
                }
                else if (tokens[1] == "jpeg_decode") {
                    run_jpeg_decode (non_ref && use_fpga,
                                     in_file, out_img, tokens);
                }
                else if (tokens[1] == "webp_save") {
                    run_webp_save (non_ref && use_fpga,
                                   out_img, out_file, tokens);
                }
                else if (tokens[1] == "webp_encode") {
                    run_webp_encode (non_ref && use_fpga,
                                     out_img, out_file, tokens);
                }

                // pure cpu
                else if (tokens[1] == "open") {
                    run_open (non_ref && use_fpga,
                              in_file, out_img, tokens);
                }
                else if (tokens[1] == "decode") {
                    run_decode (non_ref && use_fpga,
                                in_file, out_img, tokens);
                }
                else if (tokens[1] == "resize") {
                    run_resize (non_ref && use_fpga,
                                out_img, out_img, tokens);
                }
                else if (tokens[1] == "crop") {
                    run_crop (non_ref && use_fpga,
                              out_img, out_img, tokens);
                }
                else if (tokens[1] == "save") {
                    run_save (non_ref && use_fpga,
                              out_img, out_file, tokens);
                }
                else if (tokens[1] == "encode") {
                    run_encode (non_ref && use_fpga,
                                out_img, out_file, tokens);
                }
                else {
                    THROW_ERROR (-1, "invalid operation for cpu");
                }
            }
            else {
                THROW_ERROR (-1, "invalid command");
            }
        }
    }
}



void process (const bool non_ref, vector<string> &test_case) {

    if (non_ref) {
        cout << "processing" << endl;
    }
    else {
        cout << "processing by cpu" << endl;
    }

    //==========
    // get path
    //==========

    vector<string> tokens;
    boost::split (tokens, test_case.front(), boost::is_any_of ("\t "), ba::token_compress_on);
    const bfs::path in_path (tokens[1]);

    boost::split (tokens, test_case.back(), boost::is_any_of ("\t "), ba::token_compress_on);
    const bfs::path out_path (tokens[1]);
    const bfs::path ref_path = (out_path.string() + "_ref");

    if (! bfs::exists (in_path)) {
        THROW_ERROR (-1, "input not found: " + in_path.string());
    }

    cv::setNumThreads (1);

    if (bfs::is_regular_file (in_path)) {

        bpt::ptime time_start, time_stop;
        bpt::time_duration time_diff;

        //=========
        // run file
        //=========

        if (non_ref) {
            time_start = bpt::microsec_clock::local_time();

            run_test_case (non_ref, in_path, out_path, test_case);

            time_stop = bpt::microsec_clock::local_time();
            time_diff = time_stop - time_start;
            cout << "time = " << time_diff.total_milliseconds() / 1000.0 << " s" << endl ;  // time in seconds

            cout << endl;
        }
    }
    else if (bfs::is_directory (in_path)) {

        bpt::ptime time_start, time_stop;
        bpt::time_duration time_diff;

        vector<bfs::path> in_paths;
        copy (bfs::directory_iterator (in_path), bfs::directory_iterator(), back_inserter (in_paths));

        //========
        // run dir
        //========

        if (non_ref) {
            remove_all (out_path);
            create_directory (out_path);

            time_start = bpt::microsec_clock::local_time();

            #pragma omp parallel for
            for (size_t i = 0; i < in_paths.size(); ++i) {
                if (bfs::is_regular_file (in_paths[i])) {

                    bfs::path out_file (out_path.string() + "/" +
                                        in_paths[i].filename().string());

                    run_test_case (non_ref, in_paths[i], out_file, test_case);
                }
            }

            time_stop = bpt::microsec_clock::local_time();
            time_diff = time_stop - time_start;
            cout << "time = " << time_diff.total_milliseconds() / 1000.0 << " s" << endl ;  // time in seconds

            cout << endl;
        }
    }
    else {
        THROW_ERROR (-1, "not a regular file or a directory: " + in_path.string());
    }
}




int main (int argc, char *argv[]) {

    if (argc != 2) {
        THROW_ERROR (argc, "wrong number of arguments");
    }

    std::ifstream config (argv[1]);
    string line;
    vector<string> lines;

    while (config.good() && getline (config, line)) {

        if (line.find ("#") != std::string::npos) {
            line = line.substr (0, line.find ("#")); // remove comments
        }

        ba::trim (line);

        if (line.length() > 0) {

            cout << line << endl;

            if (line.find ("input") == 0) {
                lines.clear();
                lines.push_back (line);
            }
            else if (line.find ("output") == 0) {
                lines.push_back (line);
                process (true, lines);
            }
            else {
                lines.push_back (line);
            }
        }
    }

    return 0;
}

