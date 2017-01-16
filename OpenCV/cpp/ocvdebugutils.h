// A library of useful opencv debugging utilities.
// Author: Paul Foster
// Copyright: Public domain, or CC0 if that is not possible

#ifndef P_OCVDEBUGUTILS_H
#define P_OCVDEBUGUTILS_H
#ifdef __cplusplus
extern "C" {
#endif

// Returns the type string for a given type code
std::string cvtype2str(int type) {
    //inspired by a less than elegant SO post
    std::string bits, sign, ret;

    char depth = type & CV_MAT_DEPTH_MASK;//depth is last 3 bits
    char chans = 1 + (type >> CV_CN_SHIFT);
    sign = (depth & 1) ? "U" : "S";
    bits = std::to_string(1 << (3 + depth/2));
    if (depth == 7)
        bits = "USERT";
    ret = bits;
    ret += sign;
    ret += "C";
    ret += (chans+'0');

    return ret;
}

#ifdef __cplusplus
}
#endif
namespace cv{
CV_EXPORTS void debugDisplayMatches( InputArray img1, const std::vector<KeyPoint>& keypoints1,
                            InputArray img2, const std::vector<KeyPoint>& keypoints2,
                            const std::vector<DMatch>& matches1to2, InputOutputArray outImg = Mat(),
                            const Scalar& matchColor=Scalar::all(-1), const Scalar& singlePointColor=Scalar::all(-1),
                            const std::vector<char>& matchesMask=std::vector<char>(), int flags=DrawMatchesFlags::DEFAULT );
} // cv

#endif //P_OCVDEBUGUTILS_H
