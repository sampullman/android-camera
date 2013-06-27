#include <jni.h>
#include <opencv2/core/core.hpp>
#include <opencv2/core/wimage.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/legacy/legacy.hpp>
#include <vector>

#include <android/log.h>

using namespace std;
using namespace cv;

extern "C" {
JNIEXPORT void JNICALL Java_com_threeDBJ_cameraAppLib_CameraView_FindFeatures(JNIEnv* env, jobject, jint width, jint height, jbyteArray yuv, jintArray bgra) {
    jbyte* _yuv  = env->GetByteArrayElements(yuv, 0);
    jint*  _bgra = env->GetIntArrayElements(bgra, 0);

    Mat myuv(height + height/2, width, CV_8UC1, (unsigned char *)_yuv);
    Mat mbgra(height, width, CV_8UC4, (unsigned char *)_bgra);
    Mat mgray(height, width, CV_8UC1, (unsigned char *)_yuv);

    //Please make attention about BGRA byte order
    //ARGB stored in java as int array becomes BGRA at native level
    cvtColor(myuv, mbgra, CV_YUV420sp2BGR, 4);

    vector<KeyPoint> v;

    FastFeatureDetector detector(60);
    detector.detect(mgray, v);
    for( size_t i = 0; i < v.size(); i++ )
        circle(mbgra, Point(v[i].pt.x, v[i].pt.y), 10, Scalar(0,0,255,255));

    env->ReleaseIntArrayElements(bgra, _bgra, 0);
    env->ReleaseByteArrayElements(yuv, _yuv, 0);
}

  JNIEXPORT void JNICALL Java_com_threeDBJ_cameraAppLib_CameraView_ByteToInt(JNIEnv* env, jobject, jint width, jint height, jbyteArray yuv, jintArray bgra) {
    jbyte* _yuv  = env->GetByteArrayElements(yuv, 0);
    jint*  _bgra = env->GetIntArrayElements(bgra, 0);
    Mat myuv(height + height/2, width, CV_8UC1, (unsigned char *)_yuv);
    Mat mbgra(height, width, CV_8UC4, (unsigned char *)_bgra);
    cvtColor(myuv, mbgra, CV_YUV420sp2BGR, 4);

    env->ReleaseIntArrayElements(bgra, _bgra, 0);
    env->ReleaseByteArrayElements(yuv, _yuv, 0);
  }

  JNIEXPORT void JNICALL Java_com_threeDBJ_cameraAppLib_CameraView_MatchHomography(JNIEnv* env, jobject, jint width, jint height, jbyteArray yuv1, jbyteArray yuv2, jintArray bgra) {
    jbyte* _yuv1  = env->GetByteArrayElements(yuv1, 0);
    jbyte* _yuv2  = env->GetByteArrayElements(yuv2, 0);
    jint*  _bgra = env->GetIntArrayElements(bgra, 0);
    Mat myuv1(height + height/2, width, CV_8UC1, (unsigned char *)_yuv1);
    Mat myuv2(height + height/2, width, CV_8UC1, (unsigned char *)_yuv2);
    Mat mbgra(height, width, CV_8UC4, (unsigned char *)_bgra);
    Mat mgray1(height, width, CV_8UC1, (unsigned char *)_yuv1);
    Mat mgray2(height, width, CV_8UC1, (unsigned char *)_yuv2);

    Mat stitch1(height, width, CV_8UC4);
    cvtColor(myuv1, stitch1, CV_YUV420sp2BGR, 4);

    Mat stitch2(height, width, CV_8UC4);
    cvtColor(myuv2, stitch2, CV_YUV420sp2BGR, 4);

    //-- Step 1: Detect the keypoints using SURF Detector
    int minHessian = 50;

    FastFeatureDetector detector( minHessian );

    vector<KeyPoint> kp1, kp2;

    detector.detect( mgray1, kp1);
    detector.detect( mgray2, kp2);

    //-- Step 2: Calculate descriptors (feature vectors)
    BriefDescriptorExtractor extractor;

    Mat desc1, desc2;

    extractor.compute( mgray1, kp1, desc1 );
    extractor.compute( mgray2, kp2, desc2 );

    //-- Step 3: Matching descriptor vectors using FLANN matcher
    //FlannBasedMatcher matcher;
    BruteForceMatcher< Hamming > matcher;
    vector< DMatch > matches;
    matcher.match( desc1, desc2, matches );

    double max_dist = 0; double min_dist = 100;

    //-- Quick calculation of max and min distances between keypoints
    for( int i = 0; i < desc1.rows; i++ ) {
      double dist = matches[i].distance;
      if( dist < min_dist ) min_dist = dist;
      if( dist > max_dist ) max_dist = dist;
  }

  //-- Draw only "good" matches (i.e. whose distance is less than 3*min_dist )
  vector< DMatch > good_matches;

  for( int i = 0; i < desc1.rows; i++ ) {
    if( matches[i].distance < 3*min_dist ) {
      good_matches.push_back( matches[i]);
    }
  }

  Mat img_matches;
  drawMatches( mgray1, kp1, mgray2, kp2,
               good_matches, img_matches, Scalar::all(-1), Scalar::all(-1),
               vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );

  //-- Localize the object
  std::vector<Point2f> img1;
  std::vector<Point2f> img2;

  for( int i = 0; i < good_matches.size(); i++ ) {
    //-- Get the keypoints from the good matches
    Point2f p1 = kp1[good_matches[i].queryIdx].pt;
    Point2f p2 = kp2[good_matches[i].trainIdx].pt;
    img1.push_back( p1 );
    img2.push_back( p2 );
    circle(stitch1, Point(p1.x, p1.y), 10, Scalar(0, 0, 255, 255));
    circle(stitch2, Point(p2.x, p2.y), 10, Scalar(0, 0, 255, 255));
  }
  ostringstream convert;   // stream used for the conversion
  convert << img1.size() << " " << img2.size() << " good matches " << good_matches.size();
  __android_log_write(ANDROID_LOG_ERROR, "CameraApp", convert.str().c_str());

  Mat warp2;
  if(img1.size() > 3 && img2.size() > 3) {
    Mat H = findHomography( img1, img2, CV_RANSAC );

    //-- Get the corners from the img1 ( the "object" to be "detected" )
    std::vector<Point2f> img1_corners(4);
    img1_corners[0] = cvPoint(0,0); img1_corners[1] = cvPoint( mgray1.cols, 0 );
    img1_corners[2] = cvPoint( mgray1.cols, mgray1.rows ); img1_corners[3] = cvPoint( 0, mgray1.rows );
    std::vector<Point2f> img2_corners(4);

    perspectiveTransform( img1_corners, img2_corners, H.inv());
    ostringstream con;
    Point2f min(99999, 99999);
    Point2f max(-99999, -99999);
    for(size_t i=0;i<img2_corners.size();i+=1) {
      if(img2_corners[i].x < min.x) min.x = img2_corners[i].x;
      if(img2_corners[i].y < min.y) min.y = img2_corners[i].y;
      if(img2_corners[i].x > max.x) max.x = img2_corners[i].x;
      if(img2_corners[i].y > max.y) max.y = img2_corners[i].y;
    }
    double l=0, r=mbgra.cols, t=0, b=mbgra.rows;
    double l2=min.x, r2=max.x, t2=min.y, b2=max.y;
    if(l2 < 0) {
      l = -1*min.x;
      l2 = 0;
      r += l;
      r2 += l;
    }
    if(t2 < 0) {
      t = -1*min.y;
      t2 = 0;
      b += t;
      b2 += l;
    }
    Point2f sub(l2, t2);
    Point2f srcTri[3];
    Point2f dstTri[3];
    srcTri[0] = img2_corners[0];
    srcTri[1] = img2_corners[1];
    srcTri[2] = img2_corners[3];
    dstTri[0] = img2_corners[0] - sub;
    dstTri[1] = img2_corners[1] - sub;
    dstTri[2] = img2_corners[3] - sub;
    Mat warp_mat(3, 3, CV_64F);
    Mat aff = getAffineTransform( srcTri, dstTri );
    aff.copyTo(warp_mat(Rect(0, 0, 3, 2)));
    warp_mat.at<double>(2, 2) = 1;
    H = H*warp_mat;
    con.str("");
    con << l << " " << r << " "<<t<<" "<<b;
    //__android_log_write(ANDROID_LOG_ERROR, "CameraApp", con.str().c_str());
    con.str("");
    con << l2 << " " << r2 << " "<<t2<<" "<<b2;
    //__android_log_write(ANDROID_LOG_ERROR, "CameraApp", con.str().c_str());
    double w=(r > r2 ) ? r : r2, h=(b > b2) ? b : b2;
    con.str("");
    con << "w/h: "<<w << " " << h;
    __android_log_write(ANDROID_LOG_ERROR, "CameraApp", con.str().c_str());
    warp2 = Mat(h+1, w+1, CV_8UC4, Scalar(0, 0, 255, 255));
    rectangle(warp2, Rect(0, 0, warp2.cols, warp2.rows), Scalar(255, 0, 0, 255), CV_FILLED);
    Mat dest = warp2(Rect(l2, t2, r2-l2, b2-t2));
    warpPerspective(stitch2, dest, H, dest.size(), INTER_LINEAR | WARP_INVERSE_MAP, BORDER_TRANSPARENT);
    //Mat c2 = warp2(Rect(0, 0, max.x - min.x, max.y - min.y));
    //warpAffine(centerish, c2, warp_mat, c2.size());//, INTER_LINEAR | WARP_INVERSE_MAP, BORDER_TRANSPARENT);

    stitch1.copyTo(warp2(Rect(l, t, r-l, b-t)));
    resize(warp2, mbgra, mbgra.size(), 0, 0);

    perspectiveTransform( img1_corners, img2_corners, H.inv());
    Point2f min1(99999, 99999);
    Point2f max1(-99999, -99999);
    for(size_t i=0;i<img2_corners.size();i+=1) {
      if(img2_corners[i].x < min1.x) min1.x = img2_corners[i].x;
      if(img2_corners[i].y < min1.y) min1.y = img2_corners[i].y;
      if(img2_corners[i].x > max1.x) max1.x = img2_corners[i].x;
      if(img2_corners[i].y > max1.y) max1.y = img2_corners[i].y;
    }
    con.str("");
    con << "3: " << img2_corners[0] << ", " << img2_corners[1] << ", " << img2_corners[2] << ", " << img2_corners[3];
    __android_log_write(ANDROID_LOG_ERROR, "CameraApp", con.str().c_str());
    //-- Draw lines between the corners (the mapped object in the scene - image_2 )
  }

  for( int i = 0; i < good_matches.size(); i++ ) {
    //-- Get the keypoints from the good matches
    Point2f p1 = kp1[good_matches[i].queryIdx].pt;
    Point2f p2 = kp2[good_matches[i].trainIdx].pt;
    //line(mbgra, Point( p1.x/2, stitch1.rows/2 + p1.y/2), Point(stitch1.cols + p2.x / 2, stitch2.rows/2 + p2.y/2), Scalar(0, 0, 255, 255));
  }
  env->ReleaseIntArrayElements(bgra, _bgra, 0);
  env->ReleaseByteArrayElements(yuv1, _yuv1, 0);
  env->ReleaseByteArrayElements(yuv2, _yuv2, 0);

  }
}
