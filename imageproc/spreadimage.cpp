#include "spreadimage.h"
#include "GIS/shaperenderer.h"
#include "settings.h"

#include <cmath>

SpreadImage::SpreadImage(int earthRadius, int altitude)
{
    earthRadius = mEarthRadius;
    altitude = mAltitude;

    mTheta = 0.5 * SWATH / earthRadius;                                          // Maximum half-Angle subtended by Swath from Earth's centre
    mHalfChord = static_cast<int>(earthRadius * std::sin(mTheta));               // Maximum Length of chord subtended at Centre of Earth
    int h = static_cast<int>(earthRadius * std::cos(mTheta));                    // Minimum Altitude from Centre of Earth to chord from std::costd::sine
    mInc = earthRadius - h;                                                      // Maximum Distance from Arc to Chord
    mScanAngle = std::atan(mHalfChord / static_cast<double>(altitude + mInc));   // Maximum Angle subtended by half-chord from satellite
}

cv::Mat SpreadImage::stretch(const cv::Mat &image)
{
    int i, k, k2, ni, imageHeight, imageWidth, newImageWidth;
    int imageHalfWidth, newImageHalfWidth;              // centrepoints of scanlines
    int DupPix;

    imageHeight = image.size().height;
    imageWidth = image.size().width;
    imageHalfWidth = (imageWidth + 1) / 2;              // Half-width of original image ('A')

    cv::Mat strechedImage(imageHeight, static_cast<int>(imageWidth * (0.902 + std::sin(mScanAngle))), image.type());
    newImageWidth = strechedImage.size().width;
    newImageHalfWidth = (newImageWidth + 1) / 2;       // Half-Width of stretched image ('Z')

    // Create lookup table of std::sines
    memset(mLookUp, 0, sizeof(mLookUp));
    for (i = 0; i < newImageHalfWidth -1; i++)
    {
        mPhi = std::atan((i / static_cast<double>(newImageHalfWidth)) * mHalfChord / (mAltitude + mInc));
        mLookUp[i] = static_cast<int>(imageHalfWidth * (std::sin(mPhi) / std::sin(mScanAngle)));
    }

    i = 0;
    ni = 0;
    while (i < newImageHalfWidth)
    {
        mPhi = std::atan((i / static_cast<double>(newImageHalfWidth)) * mHalfChord / (mAltitude + mInc));
        k = static_cast<int>(imageHalfWidth * (std::sin(mPhi) / std::sin(mScanAngle)));
        //k = LookUp[i];
        ni = i;
        while(true)
        {
            if(mLookUp[ni] == 0 || mLookUp[ni] > k)
                break;
            ni += 1;
        }

        DupPix = ni - i;    // DupPix = number of repeated (spread) pixels
        k2 = mLookUp[ni];

        if (mLookUp[ni] == 0)
        {
            DupPix = 1;
        }

        cv::Mat scaledimageRight;
        cv::Mat scaledimageLeft;
        cv::Mat imageRight = image(cv::Rect(imageHalfWidth + k - 1, 0, 1, imageHeight));
        cv::Mat imageLeft = image(cv::Rect(imageHalfWidth - k, 0, 1, imageHeight));

        cv::resize(imageRight, scaledimageRight, cv::Size(DupPix, imageHeight));
        cv::resize(imageLeft, scaledimageLeft, cv::Size(DupPix, imageHeight));

        scaledimageRight.copyTo(strechedImage.rowRange(0, imageHeight).colRange(newImageHalfWidth + i - 1, newImageHalfWidth + i - 1 + DupPix));
        scaledimageLeft.copyTo(strechedImage.rowRange(0, imageHeight).colRange(newImageHalfWidth - i - DupPix, newImageHalfWidth - i));

        i += DupPix;
    }

    return strechedImage;
}

cv::Mat SpreadImage::mercatorProjection(const cv::Mat &image, const PixelGeolocationCalculator geolocationCalculator, ProgressCallback progressCallback)
{
    double MinX = std::min(geolocationCalculator.getTopLeftMercator().x, std::min(geolocationCalculator.getTopRightMercator().x, std::min(geolocationCalculator.getBottomLeftMercator().x, geolocationCalculator.getBottomRightMercator().x)));
    double MinY = std::min(geolocationCalculator.getTopLeftMercator().y, std::min(geolocationCalculator.getTopRightMercator().y, std::min(geolocationCalculator.getBottomLeftMercator().y, geolocationCalculator.getBottomRightMercator().y)));
    double MaxX = std::max(geolocationCalculator.getTopLeftMercator().x, std::max(geolocationCalculator.getTopRightMercator().x, std::max(geolocationCalculator.getBottomLeftMercator().x, geolocationCalculator.getBottomRightMercator().x)));
    double MaxY = std::max(geolocationCalculator.getTopLeftMercator().y, std::max(geolocationCalculator.getTopRightMercator().y, std::max(geolocationCalculator.getBottomLeftMercator().y, geolocationCalculator.getBottomRightMercator().y)));

    int width = static_cast<int>(std::abs(MaxX - MinX));
    int height = static_cast<int>(std::abs(MaxY - MinY));

    int xStart = static_cast<int>(std::min(MinX, MaxX));
    int yStart = static_cast<int>(std::min(MinY, MaxY));

    int imageWithGeorefHeight = geolocationCalculator.getGeorefMaxImageHeight() < image.size().height ? geolocationCalculator.getGeorefMaxImageHeight() : image.size().height;

    //cv::Mat newImage(height, width, image.type());
    cv::Mat newImage = cv::Mat::zeros(height, width, image.type());

    PixelGeolocationCalculator::CartesianCoordinateF p1, p2, p3;

    for (int y = 0; y < imageWithGeorefHeight - 10; y += 10)
    {
        if(progressCallback) {
            progressCallback(static_cast<float>(y) / imageWithGeorefHeight * 100.0f);
        }
        for (int x = 0; x < image.size().width - 10; x += 10)
        {
            p1 = geolocationCalculator.getMercatorAt(x, y);
            p2 = geolocationCalculator.getMercatorAt(x + 10, y);
            p3 = geolocationCalculator.getMercatorAt(x, y + 10);

            cv::Point2f srcTri[3];
            cv::Point2f dstTri[3];

            //test
            /*srcTri[0] = cv::Point2f( 0, 0 );
            srcTri[1] = cv::Point2f( 400, 0 );
            srcTri[2] = cv::Point2f( 0, 400 );

            dstTri[0] = cv::Point2f( 800, 800 );
            dstTri[1] = cv::Point2f( 400, 800 );
            dstTri[2] = cv::Point2f( 800, 400 );
            affineTransform(image, newImage, srcTri, dstTri, 0, 0);
            return newImage;*/


            srcTri[0] = cv::Point2f( x, y );
            srcTri[1] = cv::Point2f( x + 10, y );
            srcTri[2] = cv::Point2f( x, y + 10 );

            dstTri[0] = cv::Point2f(p1.x, p1.y);
            dstTri[1] = cv::Point2f(p2.x, p2.y);
            dstTri[2] = cv::Point2f(p3.x, p3.y);
            affineTransform(image, newImage, srcTri, dstTri, -xStart, -yStart);
        }
    }

    Settings &settings = Settings::getInstance();
    GIS::ShapeRenderer graticules(settings.getResourcesPath() + "ShapeFiles/ne_110m_graticules_10.shp", {200, 200, 200});
    GIS::ShapeRenderer coastLines(settings.getResourcesPath() + "ShapeFiles/ne_50m_coastline.shp", {0, 128, 128});
    GIS::ShapeRenderer country(settings.getResourcesPath() + "ShapeFiles/ne_50m_admin_0_boundary_lines_land.shp", {200, 200, 200});
    graticules.drawShapeMercator(newImage, xStart, yStart);
    coastLines.drawShapeMercator(newImage, xStart, yStart);
    country.drawShapeMercator(newImage, xStart, yStart);

    return newImage;
}

cv::Mat SpreadImage::equidistantProjection(const cv::Mat &image, const PixelGeolocationCalculator geolocationCalculator, ProgressCallback progressCallback)
{
    double MinX = std::min(geolocationCalculator.getTopLeftEquidistant().x, std::min(geolocationCalculator.getTopRightEquidistant().x, std::min(geolocationCalculator.getBottomLeftEquidistant().x, geolocationCalculator.getBottomRightEquidistant().x)));
    double MinY = std::min(geolocationCalculator.getTopLeftEquidistant().y, std::min(geolocationCalculator.getTopRightEquidistant().y, std::min(geolocationCalculator.getBottomLeftEquidistant().y, geolocationCalculator.getBottomRightEquidistant().y)));
    double MaxX = std::max(geolocationCalculator.getTopLeftEquidistant().x, std::max(geolocationCalculator.getTopRightEquidistant().x, std::max(geolocationCalculator.getBottomLeftEquidistant().x, geolocationCalculator.getBottomRightEquidistant().x)));
    double MaxY = std::max(geolocationCalculator.getTopLeftEquidistant().y, std::max(geolocationCalculator.getTopRightEquidistant().y, std::max(geolocationCalculator.getBottomLeftEquidistant().y, geolocationCalculator.getBottomRightEquidistant().y)));

    int width = static_cast<int>(std::abs(MaxX - MinX));
    int height = static_cast<int>(std::abs(MaxY - MinY));

    int xStart = static_cast<int>(std::min(MinX, MaxX));
    int yStart = static_cast<int>(std::min(MinY, MaxY));

    int imageWithGeorefHeight = geolocationCalculator.getGeorefMaxImageHeight() < image.size().height ? geolocationCalculator.getGeorefMaxImageHeight() : image.size().height;

    cv::Mat newImage = cv::Mat::zeros(height, width, image.type());

    PixelGeolocationCalculator::CartesianCoordinateF p1, p2, p3;

    for (int y = 0; y < imageWithGeorefHeight - 10; y += 10)
    {
        if(progressCallback) {
            progressCallback(static_cast<float>(y) / imageWithGeorefHeight * 100.0f);
        }
        for (int x = 0; x < image.size().width - 10; x += 10)
        {
            p1 = geolocationCalculator.getEquidistantAt(x, y);
            p2 = geolocationCalculator.getEquidistantAt(x + 10, y);
            p3 = geolocationCalculator.getEquidistantAt(x, y + 10);

            cv::Point2f srcTri[3];
            cv::Point2f dstTri[3];

            srcTri[0] = cv::Point2f( x, y );
            srcTri[1] = cv::Point2f( x + 10, y );
            srcTri[2] = cv::Point2f( x, y + 10 );

            dstTri[0] = cv::Point2f(p1.x, p1.y);
            dstTri[1] = cv::Point2f(p2.x, p2.y);
            dstTri[2] = cv::Point2f(p3.x, p3.y);
            affineTransform(image, newImage, srcTri, dstTri, -xStart, -yStart);
        }
    }

    float centerLatitude = static_cast<float>(geolocationCalculator.getCenterCoordinate().latitude * (180.0 / M_PI));
    float centerLongitude = static_cast<float>(geolocationCalculator.getCenterCoordinate().longitude * (180.0 / M_PI));

    Settings &settings = Settings::getInstance();
    GIS::ShapeRenderer coastLines(settings.getResourcesPath() + "ShapeFiles/ne_50m_coastline.shp", {0, 128, 128});
    GIS::ShapeRenderer graticules(settings.getResourcesPath() + "ShapeFiles/ne_110m_graticules_10.shp", {200, 200, 200});
    GIS::ShapeRenderer country(settings.getResourcesPath() + "ShapeFiles/ne_50m_admin_0_boundary_lines_land.shp", {200, 200, 200});
    coastLines.drawShapeEquidistant(newImage, xStart, yStart, centerLatitude, centerLongitude);
    graticules.drawShapeEquidistant(newImage, xStart, yStart, centerLatitude, centerLongitude);
    country.drawShapeEquidistant(newImage, xStart, yStart, centerLatitude, centerLongitude);

    return newImage;
}

void SpreadImage::affineTransform(const cv::Mat& src, cv::Mat& dst, const cv::Point2f source[], const cv::Point2f destination[], int originX, int originY)
{
    // Create an empty 3x1 matrix for storing original frame coordinates
    cv::Mat xOrg = cv::Mat(3, 1, CV_64FC1);

    // Create an empty 3x1 matrix for storing transformed frame coordinates
    cv::Mat xTrans = cv::Mat(3, 1, CV_64FC1);

    cv::Mat transform = cv::getAffineTransform( source, destination );
    transform.at<double>(0,2) += originX;
    transform.at<double>(1,2) += originY;

    xOrg.at<double>(0,0) = source[0].x;
    xOrg.at<double>(1,0) = source[0].y;
    xOrg.at<double>(2,0) = 1;
    xTrans = transform * xOrg;
    int minX = static_cast<const int>(std::floor(xTrans.at<double>(0,0)));
    int minY = static_cast<const int>(std::floor(xTrans.at<double>(1,0)));

    //std::cout << xTrans << std::endl;

    xOrg.at<double>(0,0) = source[1].x;
    xOrg.at<double>(1,0) = source[2].y;
    xOrg.at<double>(2,0) = 1;
    xTrans = transform * xOrg;
    int maxX = static_cast<const int>(std::ceil(xTrans.at<double>(0,0)));
    int maxY = static_cast<const int>(std::ceil(xTrans.at<double>(1,0)));

    if(maxX < minX) {
        int tmp = minX;
        minX = maxX;
        maxX = tmp;
    }

    if(maxY < minY) {
        int tmp = minY;
        minY = maxY;
        maxY = tmp;
    }

    if(minX < 0) {
        minX = 0;
    }

    if(minY < 0) {
        minY = 0;
    }


    maxX += 10; //Should be available better solution than +10
    maxY += 10;

    if(maxX >= dst.size().width) {
        maxX = dst.size().width-1;
    }

    if(maxY >= dst.size().height) {
        maxY = dst.size().height-1;
    }

    //std::cout << minX << " " << maxX << std::endl;

    cv::Mat inverseTransform;
    cv::invertAffineTransform(transform, inverseTransform);

    for(int y = minY; y < maxY; y++) {
        for(int x = minX; x < maxX; x++) {
            xOrg.at<double>(0,0) = x;
            xOrg.at<double>(1,0) = y;
            xOrg.at<double>(2,0) = 1;

            xTrans = inverseTransform * xOrg;

            // Homogeneous to cartesian transformation
            const double srcX = xTrans.at<double>(0,0);
            const double srcY = xTrans.at<double>(1,0);

            // Make sure input boundary is not exceeded
            if(srcX >= src.size().width || srcY >= src.size().height || srcX < 0 || srcY < 0)
            {
                continue;
            }

            // Make sure output boundary is not exceeded
            /*if(x >= dst.size().width || y >= dst.size().height || x < 0 || y < 0)
            {
                continue;
            }*/

            int y1 = static_cast<int>(floor(srcY));
            int y2 = y1+1;
            if(y2 >= src.size().height) {
                y2 = src.size().height - 1;
            }

            int x1 = static_cast<int>(floor(srcX));
            int x2 = x1 + 1;
            if(x2 >= src.size().width) {
                x2 = src.size().width - 1;
            }

            //Todo: order does matter when image is flipped
            cv::Vec3b interpolated = (src.at<cv::Vec3b>(y1, x2)) * ((srcX-x1)*(y2-srcY)) / ((y2-y1)*(x2-x1)) +
                (src.at<cv::Vec3b>(y2, x1)) * (((srcY-y1)*(x2-srcX)) / ((y2-y1)*(x2-x1))) +
                (src.at<cv::Vec3b>(y1, x1)) * (((x2-srcX)*(y2-srcY)) / ((y2-y1)*(x2-x1))) +
                (src.at<cv::Vec3b>(y2, x2)) * ((srcY-y1)*(srcX-x1)) / ((y2-y1)*(x2-x1));

            // Put the values of original coordinates to transformed coordinates
            //dst.at<cv::Vec3b>(y, x) = src.at<cv::Vec3b>(srcY,srcX);
            dst.at<cv::Vec3b>(y, x) = interpolated;

        }
    }
}

//This function is not finished yet
void SpreadImage::projectiveTransform(const cv::Mat& src, cv::Mat& dst, const cv::Mat &transform)
{
    // Create an empty 3x1 matrix for storing original frame coordinates
    cv::Mat xOrg = cv::Mat(3, 1, CV_64FC1);

    // Create an empty 3x1 matrix for storing transformed frame coordinates
    cv::Mat xTrans = cv::Mat(3, 1, CV_64FC1);

    // Default initialisation of output matrix
    //dst = cv::Mat::zeros(src.size(), src.type());

    // Go through entire image
    for(int i = 0; i < src.size().height; i++) {
        for(int j = 0; j < src.size().width; j++) {

        // Get current coorndinates
        xOrg.at<double>(0,0) = j;
        xOrg.at<double>(1,0) = i;
        xOrg.at<double>(2,0) = 1;

        // Get transformed coodinates
        xTrans = transform * xOrg;

        // Homogeneous to cartesian transformation
        const double newX = xTrans.at<double>(0,0) / xTrans.at<double>(2,0);
        const double newY = xTrans.at<double>(1,0) / xTrans.at<double>(2,0);

        // Make sure boundary is not exceeded
        if(newX >= dst.size().width || newY >= dst.size().height || newX < 0 || newY < 0)
        {
            continue;
        }

        int c1 = floor(newX);
        int c2 = c1 + 1;
        if(c2 >= src.size().width) {
            c2 = src.size().width - 1;
        }

        int r1 = floor(newY);
        int r2 = r1+1;
        if(r2 >= src.size().height) {
            r2 = src.size().height - 1;
        }

        //unsigned int interpolated = (src.at<uchar>(c1, r1))*((newX-c1)*(r2-newY)) / ((r2-r1)*(c2-c1)) + (src.at<uchar>(c2, r1)) * (((newY-r1)*(c2-newX))/((r2-r1)*(c2-c1))) +
        //    (src.at<uchar>(c1, r1))*(((c2-newX)*(r2-newY)) / ((r2-r1)*(c2-c1))) + (src.at<uchar>(c2, r2))*((newY-r1)*(newX-c1)) / ((r2-r1)*(c2-c1));

        // Put the values of original coordinates to transformed coordinates
        dst.at<cv::Vec3b>(newX, newY) = src.at<cv::Vec3b>(i,j);
        //dst.at<uchar>(newY, newX) = interpolated;

        }
    }
}
