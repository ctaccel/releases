// --------------------------------------------------
// flags used in flow determination:
// bool first_run      // introduced for automatic run cpu flow
// bool run_pyssim     // determine by "compre pyssim"
// bool force_output   // determine by "output /path/ true", specail for imencode and pyssim
// bool use_fpga       // determine by "fpga ops"
// --------------------------------------------------

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
#include <omp.h>
#include "ops.h"

using namespace std;
using namespace cv;

namespace bfs = boost::filesystem;
namespace ba  = boost::algorithm;
namespace bpt = boost::posix_time;


//return elapse time in ns
unsigned long long run_test_case (const bool first_run,
                    const bool force_output,
                    const bfs::path &in_file,
                    const bfs::path &out_file,
                    vector<string> &ops) {
  unsigned long long elapsed_time=0;
    try {
        Mat out_img;

        for (int i = 1; i < ops.size() - 1; i++) {

            ba::to_lower (ops[i]);

            vector<string> tokens;
            boost::split (tokens, ops[i], boost::is_any_of ("\tx "), ba::token_compress_on);
	    unsigned long long latency=0;
            if (tokens.size() > 0) {

                //======
                // fpga
                //======

                if (tokens[0] == "fpga") {
                    const bool use_fpga = true;

                    if (tokens[1] == "convert") {
                        elapsed_time += run_convert (first_run && use_fpga,
                                     in_file, out_file, tokens);
                    }
                    else if (tokens[1] == "jpeg_open") {
                        elapsed_time += run_jpeg_open (first_run && use_fpga,
                                       in_file, out_img, tokens);
                    }
                    else if (tokens[1] == "jpeg_decode") {
                        elapsed_time += run_jpeg_decode (first_run && use_fpga,
                                         in_file, out_img, tokens);
                    }
                    else if (tokens[1] == "jpeg_crop") {
                        elapsed_time += run_jpeg_crop (first_run && use_fpga,
                                       in_file, out_img, tokens);
                    }
                    else if (tokens[1] == "webp_save") {
                        elapsed_time += run_webp_save (first_run && use_fpga,
                                       out_img, out_file, tokens);
                    }
                    else if (tokens[1] == "webp_encode") {
                        elapsed_time += run_webp_encode (first_run && use_fpga,
                                         force_output,
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
                        elapsed_time += run_convert (first_run && use_fpga,
                                     in_file, out_file, tokens);
                    }
                    else if (tokens[1] == "jpeg_open") {
                        elapsed_time += run_jpeg_open (first_run && use_fpga,
                                       in_file, out_img, tokens);
                    }
                    else if (tokens[1] == "jpeg_decode") {
                        elapsed_time += run_jpeg_decode (first_run && use_fpga,
                                         in_file, out_img, tokens);
                    }
                    else if (tokens[1] == "jpeg_crop") {
                        elapsed_time += run_jpeg_crop (first_run && use_fpga,
                                       in_file, out_img, tokens);
                    }
                    else if (tokens[1] == "webp_save") {
                        elapsed_time += run_webp_save (first_run && use_fpga,
                                       out_img, out_file, tokens);
                    }
                    else if (tokens[1] == "webp_encode") {
                        elapsed_time += run_webp_encode (first_run && use_fpga,
                                         force_output,
                                         out_img, out_file, tokens);
                    }

                    // pure cpu
                    else if (tokens[1] == "open") {
                        elapsed_time += run_open (first_run && use_fpga,
                                  in_file, out_img, tokens);
                    }
                    else if (tokens[1] == "decode") {
                        elapsed_time += run_decode (first_run && use_fpga,
                                    in_file, out_img, tokens);
                    }
                    else if (tokens[1] == "resize") {
                        elapsed_time += run_resize (first_run && use_fpga,
                                    out_img, out_img, tokens);
                    }
                    else if (tokens[1] == "crop") {
                        elapsed_time += run_crop (first_run && use_fpga,
                                  out_img, out_img, tokens);
                    }
                    else if (tokens[1] == "save") {
                        elapsed_time += run_save (first_run && use_fpga,
                                  out_img, out_file, tokens);
                    }
                    else if (tokens[1] == "encode") {
                        elapsed_time += run_encode (first_run && use_fpga,
                                    force_output,
                                    out_img, out_file, tokens);
                    }
                    else if (tokens[1] == "maincolor") {
                        elapsed_time += run_main_color (first_run && use_fpga,
                                        force_output,
                                        out_img, out_file, tokens);
                    }
                    else if (tokens[1] == "sharpen") {
                        elapsed_time += run_sharpen (out_img);
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
    catch (const exception &ex) {
        cerr << ex.what() << ": " << in_file << endl;
    }
    catch (...) {
        cerr << "Unknown exception: " << in_file << endl;
    }
    return elapsed_time;
}



void run_pyssim (const bool force_output,
                 const bfs::path &in_path,
                 const bfs::path &out_path,
                 const bfs::path &ref_path,
                 const vector<string> &ops) {

    cout << "runing pyssim" << endl;

    vector<string>tokens;
    boost::split (tokens, ops[ops.size() - 2], boost::is_any_of ("\t "), ba::token_compress_on);

    string out_type;

    if (tokens[1] == "webp_save") {
        out_type = "webp";
    }
    else if (tokens[1] == "webp_encode" && force_output == true) {
        out_type = "webp";
    }
    else if (tokens[1] == "convert") {
        out_type = "jpeg";
    }
    else if (tokens[1] == "save") {
        out_type = tokens[2];
    }
    else {
        cout << "no output for this case, skip pyssim" << endl;
        return;
    }

    if (bfs::is_regular_file (in_path)) {

        stringstream ss;
        ss << "./pyssim.sh "
           << out_path.string() + "." + out_type << " "
           << ref_path.string() + "." + out_type << endl;

        const int retval = system (ss.str().c_str());

        if (retval != 0) {
            cout << "WARNING: retval = " << retval << endl;
        }
    }
    else if (is_directory (in_path)) {

        vector<bfs::path> in_paths;
        copy (bfs::directory_iterator (in_path), bfs::directory_iterator(), back_inserter (in_paths));

        #pragma omp parallel for
        for (size_t i = 0; i < in_paths.size(); ++i) {

            string out_file = in_paths[i].filename().string() + "." + out_type;

            stringstream ss;
            ss << "./pyssim.sh "
               << out_path.string() + "/" + out_file << " "
               << ref_path.string() + "/" + out_file << endl;

            const int retval = system (ss.str().c_str());

            if (retval != 0) {
                #pragma omp critical
                cout << "WARNING: retval = " << retval << endl;
            }
        }
    }
    else {
        THROW_ERROR (-1, "directory or file not found: " + ref_path.string());
    }
}



void process (const bool first_run,
              const bool pyssim,
              vector<string> &test_case) {

    if (first_run) {
        cout << "processing" << endl;
    }
    else {
        cout << "processing by cpu" << endl;
    }

    //initialize latency counter
    int max_thread = omp_get_max_threads();
    cout << "Max number of omp thread = " << max_thread <<endl;
    vector<unsigned long long> total_ns(max_thread,0);
    vector<unsigned long long> total_compute_ns(max_thread,0);
    vector<int> total_jobs(max_thread,0);

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

    // special flag for imencode and pyssim
    bool force_output = false;
    if (tokens.size() > 2) {
        force_output = (tokens[2] == "true") ? true : false;
    }


    cv::setNumThreads (1);

    //============
    // Single File
    //============

    if (bfs::is_regular_file (in_path)) {

        bpt::ptime time_start, time_stop;
        bpt::time_duration time_diff;

        //===========
        // first run
        //===========

        if (first_run) {
            time_start = bpt::microsec_clock::local_time();

            // create dir for single file, no remove
            size_t pos = out_path.string().find_last_of ("/");

            if (pos != string::npos) {
                bfs::path out_dir (out_path.string().substr (0, pos));
                create_directories (out_dir);
            }

            run_test_case (first_run, force_output, in_path, out_path, test_case);

            time_stop = bpt::microsec_clock::local_time();
            time_diff = time_stop - time_start;
            cout << "time = " << time_diff.total_milliseconds() / 1000.0 << " s" << endl ; // time in seconds

            cout << endl;
        }

        //=========
        // run cpu
        //=========

        else {

            time_start = bpt::microsec_clock::local_time();

            run_test_case (first_run, force_output, in_path, ref_path, test_case);

            time_stop = bpt::microsec_clock::local_time();
            time_diff = time_stop - time_start;
            cout << "time = " << time_diff.total_milliseconds() / 1000.0 << " s" << endl ; // time in seconds

            cout << endl;

            //===========
            // run pyssim
            //===========

            if (pyssim) {
                run_pyssim (force_output, in_path, out_path, ref_path, test_case);
            }
        }
    }

    //=====
    // Dir
    //=====

    else if (bfs::is_directory (in_path)) {

        bpt::ptime time_start, time_stop;
        bpt::time_duration time_diff;

        vector<bfs::path> in_paths;
        copy (bfs::directory_iterator (in_path), bfs::directory_iterator(), back_inserter (in_paths));

        //===========
        // first run
        //===========

        if (first_run) {
            remove_all (out_path);
            create_directories (out_path);

            time_start = bpt::microsec_clock::local_time();

            #pragma omp parallel for
            for (size_t i = 0; i < in_paths.size(); ++i) {
                if (bfs::is_regular_file (in_paths[i])) {

                    bfs::path out_file (out_path.string() + "/" +
                                        in_paths[i].filename().string());
		    //start time
		    bpt::ptime start = bpt::microsec_clock::local_time();

                    total_compute_ns[omp_get_thread_num()] += run_test_case (first_run, force_output, in_paths[i], out_file, test_case);

		    //sum latency
		    bpt::time_duration latency = bpt::microsec_clock::local_time() - start;
		    total_ns[omp_get_thread_num()] += latency.total_nanoseconds();
		    total_jobs[omp_get_thread_num()]++;

                }
            }

            time_stop = bpt::microsec_clock::local_time();
            time_diff = time_stop - time_start;
            cout << "time = " << time_diff.total_milliseconds() / 1000.0 << " s" << endl ; // time in seconds

            cout << endl;

	    //report total latency
	    int sum_jobs=0;
	    unsigned long long sum_ns=0;
	    double mean_latency_ms=0;
	    for(int i=0;i<total_ns.size();i++){
	      sum_ns += total_ns[i];
	    }
	    for(int i=0;i<total_jobs.size();i++){
	      sum_jobs += total_jobs[i];
	    }
		
	    mean_latency_ms = (double)sum_ns/1000/1000 / sum_jobs;

	    printf("Total files =  %d\n", sum_jobs);
	    printf("Mean elapsed time = %.2f ms\n", mean_latency_ms);

	    sum_ns = 0;
	    for(int i=0;i<total_compute_ns.size();i++){
	      sum_ns += total_compute_ns[i];
	    }
		
	    mean_latency_ms = (double)sum_ns/1000/1000 / sum_jobs;

	    printf("Mean elapsed time (compute only) = %.2f ms\n", mean_latency_ms);

        }

        //=========
        // run cpu
        //=========

        else {
            remove_all (ref_path);
            create_directories (ref_path);

            time_start = bpt::microsec_clock::local_time();

            #pragma omp parallel for
            for (size_t i = 0; i < in_paths.size(); ++i) {
                if (bfs::is_regular_file (in_paths[i])) {

                    const bfs::path ref_file (ref_path.string() + "/" +
                                              in_paths[i].filename().string());

                    run_test_case (first_run, force_output, in_paths[i].string(), ref_file, test_case);
                }
            }

            time_stop = bpt::microsec_clock::local_time();
            time_diff = time_stop - time_start;
            cout << "time = " << time_diff.total_milliseconds() / 1000.0 << " s" << endl ; // time in seconds

            cout << endl;

            //===========
            // run pyssim
            //===========

            if (pyssim) {
                run_pyssim (force_output, in_path, out_path, ref_path, test_case);
            }
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
    vector<string> test_case;

    while (config.good() && getline (config, line)) {

        if (line.find ("#") != std::string::npos) {
            line = line.substr (0, line.find ("#")); // remove comments
        }

        ba::trim (line);

        if (line.length() > 0) {

            cout << line << endl;

            if (line.find ("input") == 0) {
                test_case.clear();
                test_case.push_back (line);
            }
            else if (line.find ("output") == 0) {
                test_case.push_back (line);
                process (true, false, test_case);   // first_run = true, pyssim = false
            }
            else if (line.find ("compare") == 0) {
                vector<string> tokens;
                boost::split (tokens, line, boost::is_any_of ("\t "), ba::token_compress_on);
                bool pyssim = (tokens[1] == "pyssim") ? true : false;

                process (false, pyssim, test_case); // first_run = false
            }
            else {
                test_case.push_back (line);
            }
        }
    }

    return 0;
}
