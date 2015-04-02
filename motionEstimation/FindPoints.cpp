#include "FindPoints.h"

inline static double square(int a)
{
    return a * a;
}

vector<cv::Point2f> getStrongFeaturePoints(const cv::Mat& image, int number, float minQualityLevel, float minDistance) {
    /* Shi and Tomasi Feature Tracking! */

    /* Preparation: This array will contain the features found in image1. */
    vector<cv::Point2f> image_features;

    /* Preparation: BEFORE the function call this variable is the array size
     * (or the maximum number of features to find).  AFTER the function call
     * this variable is the number of features actually found.
     */
    int number_of_features = number;

    /* Actually run the Shi and Tomasi algorithm!!
     * "Image" is the input image.
     * qualityLevel specifies the minimum quality of the features (based on the eigenvalues)
     * minDistance specifies the minimum Euclidean distance between features
     * RETURNS:
     * "image_features" will contain the feature points.
     */
    cv::goodFeaturesToTrack(image, image_features, number_of_features, minQualityLevel, minDistance);
    return image_features;
}

void refindFeaturePoints(cv::Mat const& prev_image, cv::Mat const& next_image, vector<cv::Point2f> frame1_features, vector<cv::Point2f> *points1, vector<cv::Point2f> *points2){
    /* Pyramidal Lucas Kanade Optical Flow! */

    /* This array will contain the locations of the points from frame 1 in frame 2. */
    vector<cv::Point2f>  frame2_features;

    /* The i-th element of this array will be non-zero if and only if the i-th feature of
     * frame 1 was found in frame 2.
     */
    vector<unsigned char> optical_flow_found_feature;

    /* The i-th element of this array is the error in the optical flow for the i-th feature
     * of frame1 as found in frame 2.  If the i-th feature was not found (see the array above)
     * I think the i-th entry in this array is undefined.
     */
    vector<float> optical_flow_feature_error;

    /* This is the window size to use to avoid the aperture problem (see slide "Optical Flow: Overview"). */
    CvSize optical_flow_window = cvSize(15,15);

    /* 0-based maximal pyramid level number; if set to 0, pyramids are not used (single level),
     * if set to 1, two levels are used, and so on; if pyramids are passed to input then algorithm
     * will use as many levels as pyramids have but no more than maxLevel.
     * */
    int maxLevel = 10;

    /* This termination criteria tells the algorithm to stop when it has either done 20 iterations or when
     * epsilon is better than .3.  You can play with these parameters for speed vs. accuracy but these values
     * work pretty well in many situations.
     */
    cv::TermCriteria optical_flow_termination_criteria
            = cv::TermCriteria( cv::TermCriteria::COUNT | cv::TermCriteria::EPS, 20, .3 );

    /* Actually run Pyramidal Lucas Kanade Optical Flow!!
     * "prev_image" is the first frame with the known features. pyramid constructed by buildOpticalFlowPyramid()
     * "next_image" is the second frame where we want to find the first frame's features.
     * "frame1_features" are the features from the first frame.
     * "frame2_features" is the (outputted) locations of those features in the second frame.
     * "number_of_features" is the number of features in the frame1_features array.
     * "optical_flow_window" is the size of the window to use to avoid the aperture problem.
     * "maxLevel" is the maximum number of pyramids to use.  0 would be just one level.
     * "optical_flow_found_feature" is as described above (non-zero iff feature found by the flow).
     * "optical_flow_feature_error" is as described above (error in the flow for this feature).
     * "optical_flow_termination_criteria" is as described above (how long the algorithm should look).
     * "0" means disable enhancements.  (For example, the second array isn't pre-initialized with guesses.)
     */
    //TODO: improve TermCriteria. do not quit program when it is reached
    cv::calcOpticalFlowPyrLK(prev_image, next_image, frame1_features, frame2_features, optical_flow_found_feature,
                             optical_flow_feature_error, optical_flow_window, maxLevel,
                             optical_flow_termination_criteria, cv::OPTFLOW_LK_GET_MIN_EIGENVALS);


    vector<cv::Point2f>::iterator iter_f1 = frame1_features.begin();
    vector<cv::Point2f>::iterator iter_f2 = frame2_features.begin();
    for (unsigned i = 0; i < frame1_features.size(); ++i){
        if ( optical_flow_found_feature[i] == 0 ){
            frame1_features[i] = cv::Point2f(0,0);
            frame2_features[i] = cv::Point2f(0,0);
        }
        ++iter_f1;
        ++iter_f2;

        points1->push_back(frame1_features[i]);
        points2->push_back(frame2_features[i]);
    }
}

void getInliersFromMedianValue (const pair<vector<cv::Point2f>, vector<cv::Point2f> >& features, vector<cv::Point2f>* inliers1, vector<cv::Point2f>* inliers2){
    vector<double> directions;
    vector<double> lengths;

    for (unsigned int i = 0; i < features.first.size(); ++i){
        double direction = atan2( (double) (features.first[i].y - features.second[i].y) , (double) (features.first[i].x - features.second[i].x) );
        directions.push_back(direction);

        double length = sqrt( square(features.first[i].y - features.second[i].y) + square(features.first[i].x - features.second[i].x) );
        lengths.push_back(length);
    }

    sort(directions.begin(), directions.end());
    double median_direction = directions[(int)(directions.size()/2)];

    sort(lengths.begin(),lengths.end());
    double median_lenght = lengths[(int)(lengths.size()/2)];


    for(unsigned int j = 0; j < features.first.size(); ++j)
    {
        double direction = atan2( (double) (features.first[j].y - features.second[j].y) , (double) (features.first[j].x - features.second[j].x) );
        double length = sqrt( square(features.first[j].y - features.second[j].y) + square(features.first[j].x - features.second[j].x) );
        if (direction < median_direction + 0.05 && direction > median_direction - 0.05 && length < (median_lenght * 2) && length > (median_lenght * 0.5) ) {
            inliers1->push_back(features.first[j]);
            inliers2->push_back(features.second[j]);
        } else {
            inliers1->push_back(cv::Point2f(0,0));
            inliers2->push_back(cv::Point2f(0,0));
        }
    }
}


void deleteUnvisiblePoints(vector<cv::Point2f>& points1L, vector<cv::Point2f>& points1La, vector<cv::Point2f>& points1R, vector<cv::Point2f>& points1Ra, vector<cv::Point2f>& points2L, vector<cv::Point2f>& points2R, int resX, int resY){


    int size = points1L.size();
    // iterate over all points and delete points, that are not in all frames visible;
    vector<cv::Point2f>::iterator iter_c1a = points1L.begin();
    vector<cv::Point2f>::iterator iter_c1b = points1R.begin();
    vector<cv::Point2f>::iterator iter_c2a = points1La.begin();
    vector<cv::Point2f>::iterator iter_c2b = points1Ra.begin();
    vector<cv::Point2f>::iterator iter_c3a = points2L.begin();
    vector<cv::Point2f>::iterator iter_c3b = points2R.begin();
    for (unsigned int i = 0; i < size ; ++i ) {
        if (1 >= points1L[iter_c1a-points1L.begin()].x   &&
            1 >= points1L[iter_c1a-points1L.begin()].y   ||
            1 >= points1La[iter_c2a-points1La.begin()].x &&
            1 >= points1La[iter_c2a-points1La.begin()].y ||
            1 >= points2L[iter_c3a-points2L.begin()].x &&
            1 >= points2L[iter_c3a-points2L.begin()].y ||
            1 >= points1R[iter_c1b-points1R.begin()].x   &&
            1 >= points1R[iter_c1b-points1R.begin()].y   ||
            1 >= points1Ra[iter_c2b-points1Ra.begin()].x &&
            1 >= points1Ra[iter_c2b-points1Ra.begin()].y ||
            1 >= points2R[iter_c3b-points2R.begin()].x &&
            1 >= points2R[iter_c3b-points2R.begin()].y ||

            resX <= points1L[iter_c1a-points1L.begin()].x   &&
            resY <= points1L[iter_c1a-points1L.begin()].y   ||
            resX <= points1La[iter_c2a-points1La.begin()].x &&
            resY <= points1La[iter_c2a-points1La.begin()].y ||
            resX <= points2L[iter_c3a-points2L.begin()].x &&
            resY <= points2L[iter_c3a-points2L.begin()].y ||
            resX <= points1R[iter_c1b-points1R.begin()].x   &&
            resY <= points1R[iter_c1b-points1R.begin()].y   ||
            resX <= points1Ra[iter_c2b-points1Ra.begin()].x &&
            resY <= points1Ra[iter_c2b-points1Ra.begin()].y ||
            resX <= points2R[iter_c3b-points2R.begin()].x &&
            resY <= points2R[iter_c3b-points2R.begin()].y )
        {
            points1L.erase(iter_c1a);
            points1R.erase(iter_c1b);
            points1La.erase(iter_c2a);
            points1Ra.erase(iter_c2b);
            points2L.erase(iter_c3a);
            points2R.erase(iter_c3b);
        } else
        {
            ++iter_c1a;
            ++iter_c1b;
            ++iter_c2a;
            ++iter_c2b;
            ++iter_c3a;
            ++iter_c3b;
        }
    }
}

void deleteZeroLines(vector<cv::Point2f>& points1, vector<cv::Point2f>& points2){
    int size = points1.size();
    vector<cv::Point2f>::iterator iter_p1 = points1.begin();
    vector<cv::Point2f>::iterator iter_p2 = points2.begin();
    for (unsigned int i = 0; i < size; ++i) {
        if ((0 == points1[iter_p1-points1.begin()].x && 0 == points1[iter_p1-points1.begin()].y) ||
            (0 == points2[iter_p2-points2.begin()].x && 0 == points2[iter_p2-points2.begin()].y)){
            points1.erase(iter_p1);
            points2.erase(iter_p2);
        } else {
            ++iter_p1;
            ++iter_p2;
        }
    }
}

void deleteZeroLines(vector<cv::Point2f>& points1La, vector<cv::Point2f>& points1Lb, vector<cv::Point2f>& points1Ra,
                     vector<cv::Point2f>& points1Rb, vector<cv::Point2f>& points2L, vector<cv::Point2f>& points2R){
    int size = points1La.size();
    vector<cv::Point2f>::iterator iter_p1La = points1La.begin();
    vector<cv::Point2f>::iterator iter_p1Lb = points1Lb.begin();
    vector<cv::Point2f>::iterator iter_p1Ra = points1Ra.begin();
    vector<cv::Point2f>::iterator iter_p1Rb = points1Rb.begin();
    vector<cv::Point2f>::iterator iter_p2L  = points2L.begin();
    vector<cv::Point2f>::iterator iter_p2R  = points2R.begin();
    for (unsigned int i = 0; i < size; ++i) {
        if ((0 == points1La[iter_p1La-points1La.begin()].x && 0 == points1La[iter_p1La-points1La.begin()].y) ||
            (0 == points1Lb[iter_p1Lb-points1Lb.begin()].x && 0 == points1Lb[iter_p1Lb-points1Lb.begin()].y) ||
            (0 == points1Ra[iter_p1Ra-points1Ra.begin()].x && 0 == points1Ra[iter_p1Ra-points1Ra.begin()].y) ||
            (0 == points1Rb[iter_p1Rb-points1Rb.begin()].x && 0 == points1Rb[iter_p1Rb-points1Rb.begin()].y) ||
            (0 == points2L[iter_p2L-points2L.begin()].x && 0 == points2L[iter_p2L-points2L.begin()].y) ||
            (0 == points2R[iter_p2R-points2R.begin()].x && 0 == points2R[iter_p2R-points2R.begin()].y))
        {
            points1La.erase(iter_p1La);
            points1Lb.erase(iter_p1Lb);
            points1Ra.erase(iter_p1Ra);
            points1Rb.erase(iter_p1Rb);
            points2L.erase(iter_p2L);
            points2R.erase(iter_p2R);
        } else {
            ++iter_p1La ;
            ++iter_p1Lb ;
            ++iter_p1Ra ;
            ++iter_p1Rb ;
            ++iter_p2L  ;
            ++iter_p2R  ;
        }
    }
}

void deleteZeroLines(vector<cv::Point2f>& points1L, vector<cv::Point2f>& points1R,
                     vector<cv::Point2f>& points2L, vector<cv::Point2f>& points2R){
    int size = points1L.size();
    vector<cv::Point2f>::iterator iter_p1L = points1L.begin();
    vector<cv::Point2f>::iterator iter_p1R = points1R.begin();
    vector<cv::Point2f>::iterator iter_p2L  = points2L.begin();
    vector<cv::Point2f>::iterator iter_p2R  = points2R.begin();
    for (unsigned int i = 0; i < size; ++i) {
        if ((0 == points1L[iter_p1L-points1L.begin()].x && 0 == points1L[iter_p1L-points1L.begin()].y) ||
            (0 == points1R[iter_p1R-points1R.begin()].x && 0 == points1R[iter_p1R-points1R.begin()].y) ||
            (0 == points2L[iter_p2L-points2L.begin()].x && 0 == points2L[iter_p2L-points2L.begin()].y) ||
            (0 == points2R[iter_p2R-points2R.begin()].x && 0 == points2R[iter_p2R-points2R.begin()].y))
        {
            points1L.erase(iter_p1L);
            points1R.erase(iter_p1R);
            points2L.erase(iter_p2L);
            points2R.erase(iter_p2R);
        } else {
            ++iter_p1L ;
            ++iter_p1R ;
            ++iter_p2L  ;
            ++iter_p2R  ;
        }
    }
}

void normalizePoints(const cv::Mat& KLInv, const cv::Mat& KRInv, const vector<cv::Point2f>& pointsL, const vector<cv::Point2f>& pointsR, vector<cv::Point2f>& normPointsL, vector<cv::Point2f>& normPointsR){

    vector<cv::Point3f> pointsL_h, pointsR_h;
    cv::convertPointsToHomogeneous(pointsL, pointsL_h);
    cv::convertPointsToHomogeneous(pointsR, pointsR_h);

    KLInv.convertTo(KLInv, CV_64F);
    KRInv.convertTo(KRInv, CV_64F);

    for(unsigned int i = 0; i < pointsL.size(); ++i){
        cv::Mat matPointL_h(pointsL_h[i]);
        matPointL_h.convertTo(matPointL_h, CV_64F);

        cv::Mat matPointR_h(pointsR_h[i]);
        matPointR_h.convertTo(matPointR_h, CV_64F);

        pointsL_h[i] = cv::Point3f(cv::Mat(KLInv * matPointL_h));
        pointsR_h[i] = cv::Point3f(cv::Mat(KRInv * matPointR_h));
    }
    cv::convertPointsFromHomogeneous(pointsL_h, normPointsL);
    cv::convertPointsFromHomogeneous(pointsR_h, normPointsR);
}


void findCorresPoints_LucasKanade(const cv::Mat& frame_L1, const cv::Mat& frame_R1, const cv::Mat& frame_L2, const cv::Mat& frame_R2, vector<cv::Point2f> *points_L1, vector<cv::Point2f> *points_R1, vector<cv::Point2f> *points_L2, vector<cv::Point2f> *points_R2){
    // find corresponding points
    vector<cv::Point2f> points_L1_temp, points_R1_temp, points_L1a_temp, points_R1a_temp, points_L2_temp, points_R2_temp;
    vector<cv::Point2f> features = getStrongFeaturePoints(frame_L1, 500,0.001,5);

    if (0 == features.size()){
        return;
    }

    refindFeaturePoints(frame_L1, frame_R1, features, &points_L1_temp, &points_R1_temp);
    refindFeaturePoints(frame_L1, frame_L2, points_L1_temp, &points_L1a_temp, &points_L2_temp);
    refindFeaturePoints(frame_R1, frame_R2, points_R1_temp, &points_R1a_temp, &points_R2_temp);

    drawPoints(frame_L1, features, "feaures found" , cv::Scalar(2,55,212));

    cout << "size of points: " << points_L1_temp.size() << ", " << points_R1_temp.size()  << ", " << points_L2_temp.size() << ", " << points_R2_temp.size() << endl;

    // delete in all frames points, that are not visible in each frames
    deleteUnvisiblePoints(points_L1_temp, points_L1a_temp, points_R1_temp, points_R1a_temp, points_L2_temp, points_R2_temp, frame_L1.cols, frame_L1.rows);

    cout << "size of points: " << points_L1_temp.size() << ", " << points_R1_temp.size()  << ", " << points_L2_temp.size() << ", " << points_R2_temp.size() << endl;

    for (unsigned int i = 0; i < points_L1_temp.size(); ++i){
        points_L1->push_back(points_L1_temp[i]);
        points_R1->push_back(points_R1_temp[i]);
        points_L2->push_back(points_L2_temp[i]);
        points_R2->push_back(points_R2_temp[i]);
    }
}

