#include "opencv2/features2d.hpp" //drawMatches
#include "opencv2/highgui.hpp" //imshow
#include "opencv2/imgproc.hpp" //cvtColor
#include <iostream>
namespace cv{

// A wrapper around drawMatches that actually checks for color images, etc
CV_EXPORTS void debugDisplayMatches( InputArray img1, const std::vector<KeyPoint>& keypoints1,
                            InputArray img2, const std::vector<KeyPoint>& keypoints2,
                            const std::vector<DMatch>& matches1to2, InputOutputArray outImg,
                            const Scalar& matchColor, const Scalar& singlePointColor,
                            const std::vector<char>& matchesMask, int flags)
{
    Mat i1g,i2g,matchImage;

    cvtColor(img1, i1g, cv::COLOR_BGR2GRAY);
    cvtColor(img2, i2g, cv::COLOR_BGR2GRAY);
    drawMatches(i1g, keypoints1, i2g, keypoints2, matches1to2, matchImage, matchColor, singlePointColor, matchesMask, flags);
    namedWindow("matches",CV_WINDOW_NORMAL | CV_WINDOW_KEEPRATIO | CV_GUI_EXPANDED);
    imshow("matches", matchImage);
    std::cout << "Press SPACE to continue" << std::endl;
    while(waitKey(0)!=' ');
}

} // cv
