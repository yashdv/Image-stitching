#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/nonfree/features2d.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <opencv2/legacy/legacy.hpp>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <sys/time.h>

using namespace std;
using namespace cv;

char window_name[] = "Feature Detection and Matching";
char detector_name[128];
char extractor_name[128];
char matcher_name[128];
char panorama[128];
timeval t1, t2, t3, t4, t5, t6;

unsigned long long int timex(timeval &t)
{
    unsigned long long int ret = t.tv_sec*1000000ULL + t.tv_usec;
    return ret;
}

void printParams( cv::Algorithm* algorithm )
{
    vector<string> parameters;
    algorithm->getParams(parameters);

    for (int i=0; i<parameters.size(); i++)
    {
        string which_type;
        switch (algorithm->paramType(parameters[i]))
        {
            case cv::Param::BOOLEAN:
                which_type = "bool";
                break;
            case cv::Param::INT:
                which_type = "int";
                break;
            case cv::Param::REAL:
                which_type = "real (double)";
                break;
            case cv::Param::STRING:
                which_type = "string";
                break;
            case cv::Param::MAT:
                which_type = "Mat";
                break;
            case cv::Param::ALGORITHM:
                which_type = "Algorithm";
                break;
            case cv::Param::MAT_VECTOR:
                which_type = "Mat vector";
                break;
        }
        cout << "Parameter '" << parameters[i];
        cout << "' type=" << which_type;
        cout << " help=" << algorithm->paramHelp(parameters[i]) << endl;
    }
}

class FeatureX
{
    public:
        Mat img1, img2;
        Mat imgc1, imgc2;
        Mat des1, des2;
        vector<KeyPoint> keypoints1, keypoints2, kp1, kp2;
        vector< vector<Point2f> >points;
        vector< vector<DMatch> > matches1, matches2;
        vector< DMatch > sym;
        float ratio;
        double epi_line_dist;
        double confidence;
        double epi2;
        double con2;

        FeatureX(const char *i1, const char *i2)
        {
            img1 = imread(i1,0);
            img2 = imread(i2,0);

            if(!img1.data || !img2.data)
            {
                puts("Error: Cant read files");
                exit(-1);
            }

            imgc1 = imread(i1);
            imgc2 = imread(i2);

            points = vector< vector<Point2f> >(2);
            ratio = 0.65;
            epi_line_dist = 2.0;
            confidence = 0.99;
        }

        void get_features_and_matches()
        {
            gettimeofday(&t1, 0);
            Ptr<FeatureDetector> detector = FeatureDetector::create(detector_name);
            //printParams(detector);
            //exit(-1);
            //if(strcmp(detector_name, "SURF") == 0)
            //	detector->set("hessianThreshold",2400);
            detector->detect(img1, keypoints1);
            detector->detect(img2, keypoints2);
            gettimeofday(&t2, 0);

            Ptr<DescriptorExtractor> extractor = DescriptorExtractor::create(extractor_name);
            extractor->compute(img1, keypoints1, des1);
            extractor->compute(img2, keypoints2, des2);
            gettimeofday(&t3, 0);

            Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create(matcher_name);
            matcher->knnMatch(des1, des2, matches1, 2);
            matcher->knnMatch(des2, des1, matches2, 2);
            gettimeofday(&t4, 0);

        }

        void ratioTest(vector< vector<DMatch> > &m) 
        {
            int cnt = 0;
            for(vector< vector<DMatch> >::iterator it=m.begin(); it!=m.end(); ++it)
            {
                // if 2 NN has been identified
                if (it->size() > 1)
                {
                    if ((*it)[0].distance / (*it)[1].distance > ratio) 
                        it->clear(), ++cnt;
                } 
                else
                    it->clear();
            }
            //cout << "cnt = " << cnt << endl;
        }

        void distanceTest(vector< vector<DMatch> > &m)
        {
            int minn = 1e9;
            for(vector< vector<DMatch> >::iterator it=m.begin(); it!=m.end(); ++it)
                if(it->size() > 1)
                    for(vector<DMatch>::iterator it2=it->begin(); it2!=it->end(); ++it2)
                        minn = min(minn, (int)it2->distance);

            for(vector< vector<DMatch> >::iterator it=m.begin(); it!=m.end(); ++it)
                if(it->size() > 1)
                    if ((*it)[0].distance > 2*minn && (*it)[1].distance > 2*minn)
                        it->size();
        }

        void do_symm_matching()
        {
            for(vector<vector<DMatch> >::const_iterator it1 = matches1.begin(); it1 != matches1.end(); ++it1)
            {
                if(it1->size() < 2)
                    continue;
                for(vector<vector<DMatch> >::const_iterator it2 = matches2.begin(); it2 != matches2.end(); ++it2)
                {
                    if(it2->size() < 2)
                        continue;

                    if((*it1)[0].queryIdx == (*it2)[0].trainIdx && (*it2)[0].queryIdx == (*it1)[0].trainIdx)
                    {
                        sym.push_back(DMatch((*it1)[0].queryIdx, (*it1)[0].trainIdx, (*it1)[0].distance));
                        break;
                    }
                }
            }

            for(vector<DMatch>::const_iterator it = sym.begin(); it!=sym.end(); ++it)
            {
                points[0].push_back(Point2f(keypoints1[it->queryIdx].pt.x,
                                            keypoints1[it->queryIdx].pt.y));
                points[1].push_back(Point2f(keypoints2[it->trainIdx].pt.x,
                                            keypoints2[it->trainIdx].pt.y));

                kp1.push_back(keypoints1[it->queryIdx]);
                kp2.push_back(keypoints2[it->trainIdx]);
            }
        }

        void find_fundamental_matrix(Mat &F, vector<uchar> &inliers)
        {
            inliers = vector<uchar>(points[0].size(), 0);

            if(points[0].size() <= 4)
            {
                puts("too few points");
                cout << "Only " << points[0].size() << " points detected!" << endl;
                exit(-1);
            }

            cout << "Number of Good points Detected = "<< points[0].size() << "##" << endl;
            F = findFundamentalMat(Mat(points[0]), Mat(points[1]), inliers, CV_FM_RANSAC, epi_line_dist, confidence);
        }

        void find_homography(Mat &homography, vector<uchar> &inliers)
        {
            Mat F = findFundamentalMat(Mat(points[1]), Mat(points[0]), inliers, CV_FM_RANSAC, 2, 0.8);
            homography = findHomography(Mat(points[1]), Mat(points[0]), inliers, CV_RANSAC, 1);

            Mat result;

            Mat im1 = imgc1.clone();
            Mat im2 = imgc2.clone();

            warpPerspective(im2, result, homography, Size(img1.cols+img2.cols, max(img1.rows, img2.rows)));

            imwrite("./upload/hom.png", result);

            if(strcmp(panorama,"Yes") == 0)
            {
                Mat half(result, Rect(0, 0, img1.cols, img1.rows));
                im1.copyTo(half);
                imwrite("./upload/pan.png", result);
            }
        }

        double find_error(Mat &F)
        {
            int l = points[0].size();
            double err = 0;
            Mat x2t(1, 3, CV_64FC1), x1(3, 1, CV_64FC1), prod;

            x2t.at<double>(0,2) = 1;
            x1.at<double>(2,0) = 1;

            for(int i=0; i<l; ++i)
            {
                x2t.at<double>(0,0) = points[1][i].x;
                x2t.at<double>(0,1) = points[1][i].y;

                x1.at<double>(0,0) = points[0][i].x;
                x1.at<double>(1,0) = points[0][i].y;

                prod = x2t * F * x1;
                prod /= norm(F * x1);
                err += fabs(prod.at<double>(0,0));
            }

            return err;
        }


        void find_epipolar_lines(vector< vector<Vec3f> > &lines, Mat &F)
        {
            for(int i=0; i<2; ++i)
                computeCorrespondEpilines(points[i], i+1, F, lines[i]);
        }

        void draw_match(Mat &out)
        {
            Mat out1,out2;
            drawKeypoints(img1, kp1, out1);
            drawKeypoints(img2, kp2, out2);
            //	drawMatches(img1, kp1, img2, kp2, sym, out);

            out = Mat::zeros(max(img1.rows,img2.rows), img1.cols+img2.cols, CV_8UC3);
            Mat h1(out, Rect(0, 0, img1.cols, img1.rows));
            Mat h2(out, Rect(img1.cols, 0, img2.cols, img2.rows));
            out2.copyTo(h2);
            out1.copyTo(h1);		
            int l = kp1.size();

            RNG rng(12345);

            for(int j=0; j<l; ++j)
            {
                line(out,
                     kp1[j].pt,
                     Point(img1.cols+kp2[j].pt.x,kp2[j].pt.y), 
                     Scalar(rng.uniform(0,255),rng.uniform(0,255),rng.uniform(0,255)));
                //	circle(fir, points[0][j], 11, Scalar(0,0,0), -1);
                //	circle(fir, points[0][j], 5, Scalar(255,255,255), -1);
                //	circle(sec, points[1][j], 11, Scalar(0,0,0), -1);
                //	circle(sec, points[1][j], 5, Scalar(255,255,255), -1);
            }
        }
};

void drawEpiLines(Mat &out,
                  Mat &image2,
                  Mat &image1,
                  vector<Vec3f> &lines,
                  vector<uchar> &inliers,
                  vector< vector<Point2f> > &points)
{
    Mat fir = image1;
    Mat sec = image2;
    int l = lines.size();
    int cnt = 10;

    for(int j=0; j<l && cnt; ++j)
    {

        if(!inliers[j])
            continue;

        line(sec,
             Point(0, -lines[j][2]/lines[j][1]),
             Point(image2.cols, -(lines[j][2]+lines[j][0]*image2.cols)/lines[j][1]),
             Scalar(255,255,255));

        circle(fir, points[0][j], 11, Scalar(0,0,0), -1);
        circle(fir, points[0][j], 5, Scalar(255,255,255), -1);
        circle(sec, points[1][j], 11, Scalar(0,0,0), -1);
        circle(sec, points[1][j], 5, Scalar(255,255,255), -1);

        --cnt;
    }

    out = Mat::zeros(max(image1.rows,image2.rows), image1.cols+image2.cols, CV_8UC3);
    Mat h1(out, Rect(0, 0, image1.cols, image1.rows));
    Mat h2(out, Rect(image1.cols, 0, image2.cols, image2.rows));
    sec.copyTo(h2);
    fir.copyTo(h1);
    //out = h2;
}

int main(int argc, const char* argv[])
{
    if(argc != 7)
    {
        puts("Usage: ./a.out <img1> <img2> <detector> <extractor> <matcher> <panorama>");
        return 0;
    }

    strcpy(detector_name, argv[3]);
    strcpy(extractor_name, argv[4]);
    strcpy(matcher_name, argv[5]);
    strcpy(panorama, argv[6]);

    Mat F, out, out2, H;
    vector< vector<Vec3f> >lines(2);
    vector<uchar> inliers;

    // Need to be called for non free Algorithm, else ::create won't work
    initModule_nonfree();

    FeatureX fd(argv[1], argv[2]);
    fd.get_features_and_matches();

    fd.distanceTest(fd.matches1);
    fd.distanceTest(fd.matches2);
    fd.ratioTest(fd.matches1);
    fd.ratioTest(fd.matches2);
    fd.do_symm_matching();

    gettimeofday(&t5, 0);
    fd.find_fundamental_matrix(F, inliers);
    fd.find_homography(H, inliers);
    gettimeofday(&t6, 0);

    double err = fd.find_error(F);
    fd.find_epipolar_lines(lines, F);

    cout << "F = \n" << F << "##" << endl << endl;
    cout << "Reprojection Error = " << err << "##" << endl << endl;

    float detT   = (timex(t2) - timex(t1)) / 1000.0;
    float extT   = (timex(t3) - timex(t2)) / 1000.0;
    float matchT = (timex(t3) - timex(t2)) / 1000.0;
    float FT     = (timex(t6) - timex(t5)) / 1000.0;

    cout << "Detection time  = " << detT << "#" << endl;
    cout << "Extraction time = " << extT << "#" << endl;
    cout << "Matching time   = " << matchT << "#" << endl;
    cout << "Time to find F  = " << FT << "#" << endl << endl;

    fd.draw_match(out);
    drawEpiLines(out2, fd.imgc2, fd.imgc1, lines[0], inliers, fd.points);
    //drawEpiLines(out2, fd.imgc1, fd.imgc2, lines[1], inliers, fd.points[1]);

    imwrite("./upload/matches.png",out);
    imwrite("./upload/epilines.png",out2);

    return 0;
}
