#include "shaperenderer.h"
#include "pixelgeolocationcalculator.h"
#include <vector>


GIS::ShapeRenderer::ShapeRenderer(const std::string shapeFile, const cv::Scalar &color, int earthRadius, int altitude)
    : ShapeReader(shapeFile)
    , mColor(color)
    , mEarthRadius(earthRadius)
    , mAltitude(altitude)
    , mThicknes(5)
    , mPointRadius(10)
    , mFontHeight(40)
    , mFontLineWidth(2)
{

}

void GIS::ShapeRenderer::addNumericFilter(const std::string name, int value)
{
    mfilter.insert(std::make_pair(name, value));
}

void GIS::ShapeRenderer::setTextFieldName(const std::string &name)
{
    mTextFieldName = name;
}

void GIS::ShapeRenderer::drawShapeMercator(cv::Mat &src, float xStart, float yStart, float scale)
{
    if(!load()) {
        return;
    }

    if(getShapeType() == ShapeReader::ShapeType::stPolyline) {
        auto recordIterator = getRecordIterator();

        if(recordIterator) {
            for(recordIterator->begin(); *recordIterator != recordIterator->end(); ++(*recordIterator)) {
                auto polyLineIterator = getPolyLineIterator(*recordIterator);

                if(polyLineIterator) {
                    std::vector<cv::Point> polyLines;
                    for(polyLineIterator->begin(); *polyLineIterator != polyLineIterator->end(); ++(*polyLineIterator)) {
                        //std::cout << polyLineIterator->point.x << " " << polyLineIterator->point.y << std::endl;

                        PixelGeolocationCalculator::CartesianCoordinateF coordinate = PixelGeolocationCalculator::coordinateToMercatorProjection<float>({polyLineIterator->point.y, polyLineIterator->point.x, 0}, mEarthRadius + mAltitude, scale);

                        coordinate.x += -xStart;
                        coordinate.y += -yStart;

                        polyLines.push_back(cv::Point2d(coordinate.x, coordinate.y));
                    }

                    if(polyLines.size() > 1) {
                        cv::polylines(src, polyLines, false, mColor, mThicknes);
                    }
                }
            }
        }
    } else if(getShapeType() == ShapeReader::ShapeType::stPoint) {
        auto recordIterator = getRecordIterator();

        if(mfilter.size() == 0) {
            if(recordIterator) {
                for(recordIterator->begin(); *recordIterator != recordIterator->end(); ++(*recordIterator)) {
                    ShapeReader::Point point(*recordIterator);

                    PixelGeolocationCalculator::CartesianCoordinateF coordinate = PixelGeolocationCalculator::coordinateToMercatorProjection<float>({point.y, point.x, 0}, mEarthRadius + mAltitude, scale);
                    coordinate.x += -xStart;
                    coordinate.y += -yStart;

                    cv::circle(src, cv::Point2d(coordinate.x, coordinate.y), mPointRadius, mColor, cv::FILLED);
                    cv::circle(src, cv::Point2d(coordinate.x, coordinate.y), mPointRadius, cv::Scalar(0,0,0), 1);
                }
            }
        } else {
            const DbFileReader &dbFilereader = getDbFilereader();
            const std::vector<DbFileReader::Field> fieldAttributes = dbFilereader.getFieldAttributes();

            if(recordIterator && hasDbFile()) {
                uint32_t i = 0;
                for(recordIterator->begin(); *recordIterator != recordIterator->end(); ++(*recordIterator), ++i) {
                    ShapeReader::Point point(*recordIterator);
                    std::vector<std::string> fieldValues = dbFilereader.getFieldValues(i);

                    PixelGeolocationCalculator::CartesianCoordinateF coordinate = PixelGeolocationCalculator::coordinateToMercatorProjection<float>({point.y, point.x, 0}, mEarthRadius + mAltitude, scale);
                    coordinate.x += -xStart;
                    coordinate.y += -yStart;

                    bool drawName = false;
                    size_t namePos = 0;

                    for(size_t n = 0; n < fieldAttributes.size(); n++) {
                        if(mfilter.count(fieldAttributes[n].fieldName) == 1) {
                            int population = 0;
                            try {
                                population = std::stoi(fieldValues[n]);
                            } catch (...) {
                                continue;
                            }

                            if(population >= mfilter[fieldAttributes[n].fieldName]) {
                                cv::circle(src, cv::Point2d(coordinate.x, coordinate.y), mPointRadius, mColor, cv::FILLED);
                                cv::circle(src, cv::Point2d(coordinate.x, coordinate.y), mPointRadius, cv::Scalar(0,0,0), 1);

                                drawName = true;
                            }
                        }

                        if(std::string(fieldAttributes[n].fieldName) == mTextFieldName) {
                            namePos = n;
                        }
                    }

                    if(drawName) {
                        double fontScale = cv::getFontScaleFromHeight(cv::FONT_ITALIC, mFontHeight, mFontLineWidth);
                        int baseLine;
                        cv::Size size = cv::getTextSize(fieldValues[namePos], cv::FONT_ITALIC, fontScale, mFontLineWidth, &baseLine);
                        cv::putText(src, fieldValues[namePos], cv::Point2d(coordinate.x - (size.width/2), coordinate.y - size.height + baseLine), cv::FONT_ITALIC, fontScale, cv::Scalar(0,0,0), mFontLineWidth+1, cv::LINE_AA);
                        cv::putText(src, fieldValues[namePos], cv::Point2d(coordinate.x - (size.width/2), coordinate.y - size.height + baseLine), cv::FONT_ITALIC, fontScale, mColor, mFontLineWidth, cv::LINE_AA);
                    }
                }
            }
        }
    }
}

void GIS::ShapeRenderer::drawShapeEquidistant(cv::Mat &src, float xStart, float yStart, float centerLatitude, float centerLongitude, float scale)
{
    if(!load()) {
        return;
    }

    if(getShapeType() == ShapeReader::ShapeType::stPolyline) {
        auto recordIterator = getRecordIterator();

        if(recordIterator) {
            for(recordIterator->begin(); *recordIterator != recordIterator->end(); ++(*recordIterator)) {
                auto polyLineIterator = getPolyLineIterator(*recordIterator);

                if(polyLineIterator) {
                    std::vector<cv::Point> polyLines;
                    for(polyLineIterator->begin(); *polyLineIterator != polyLineIterator->end(); ++(*polyLineIterator)) {
                        //std::cout << polyLineIterator->point.x << " " << polyLineIterator->point.y << std::endl;

                        PixelGeolocationCalculator::CartesianCoordinateF coordinate = PixelGeolocationCalculator::coordinateToAzimuthalEquidistantProjection<float>({polyLineIterator->point.y, polyLineIterator->point.x, 0}, {centerLatitude, centerLongitude, 0}, mEarthRadius + mAltitude, scale);

                        coordinate.x += -xStart;
                        coordinate.y += -yStart;

                        if(equidistantCheck(polyLineIterator->point.y, polyLineIterator->point.x, centerLatitude, centerLongitude)) {
                            polyLines.push_back(cv::Point2d(coordinate.x, coordinate.y));
                        }
                    }

                    if(polyLines.size() > 1) {
                        cv::polylines(src, polyLines, false, mColor, mThicknes);
                    }
                }
            }
        }
    } else if(getShapeType() == ShapeReader::ShapeType::stPoint) {
        auto recordIterator = getRecordIterator();

        if(mfilter.size() == 0) {
            if(recordIterator) {
                for(recordIterator->begin(); *recordIterator != recordIterator->end(); ++(*recordIterator)) {
                    ShapeReader::Point point(*recordIterator);

                    PixelGeolocationCalculator::CartesianCoordinateF coordinate = PixelGeolocationCalculator::coordinateToAzimuthalEquidistantProjection<float>({point.y, point.x, 0}, {centerLatitude, centerLongitude, 0}, mEarthRadius + mAltitude, scale);
                    coordinate.x += -xStart;
                    coordinate.y += -yStart;

                    if(equidistantCheck(point.y, point.x, centerLatitude, centerLongitude) == false) {
                        continue;
                    }

                    cv::circle(src, cv::Point2d(coordinate.x, coordinate.y), mPointRadius, mColor, cv::FILLED);
                    cv::circle(src, cv::Point2d(coordinate.x, coordinate.y), mPointRadius, cv::Scalar(0,0,0), 1);
                }
            }
        } else {
            const DbFileReader &dbFilereader = getDbFilereader();
            const std::vector<DbFileReader::Field> fieldAttributes = dbFilereader.getFieldAttributes();

            if(recordIterator && hasDbFile()) {
                uint32_t i = 0;
                for(recordIterator->begin(); *recordIterator != recordIterator->end(); ++(*recordIterator), ++i) {
                    ShapeReader::Point point(*recordIterator);
                    std::vector<std::string> fieldValues = dbFilereader.getFieldValues(i);

                    PixelGeolocationCalculator::CartesianCoordinateF coordinate = PixelGeolocationCalculator::coordinateToAzimuthalEquidistantProjection<float>({point.y, point.x, 0}, {centerLatitude, centerLongitude, 0}, mEarthRadius + mAltitude, scale);
                    coordinate.x += -xStart;
                    coordinate.y += -yStart;

                    if(equidistantCheck(point.y, point.x, centerLatitude, centerLongitude) == false) {
                        continue;
                    }

                    bool drawName = false;
                    size_t namePos = 0;

                    for(size_t n = 0; n < fieldAttributes.size(); n++) {
                        if(mfilter.count(fieldAttributes[n].fieldName) == 1) {
                            int population = 0;
                            try {
                                population = std::stoi(fieldValues[n]);
                            } catch (...) {
                                continue;
                            }

                            if(population >= mfilter[fieldAttributes[n].fieldName]) {
                                cv::circle(src, cv::Point2d(coordinate.x, coordinate.y), mPointRadius, mColor, cv::FILLED);
                                cv::circle(src, cv::Point2d(coordinate.x, coordinate.y), mPointRadius, cv::Scalar(0,0,0), 1);

                                drawName = true;
                            }
                        }

                        if(std::string(fieldAttributes[n].fieldName) == mTextFieldName) {
                            namePos = n;
                        }
                    }

                    if(drawName) {
                        double fontScale = cv::getFontScaleFromHeight(cv::FONT_ITALIC, mFontHeight, mFontLineWidth);
                        int baseLine;
                        cv::Size size = cv::getTextSize(fieldValues[namePos], cv::FONT_ITALIC, fontScale, mFontLineWidth, &baseLine);
                        cv::putText(src, fieldValues[namePos], cv::Point2d(coordinate.x - (size.width/2), coordinate.y - size.height + baseLine), cv::FONT_ITALIC, fontScale, cv::Scalar(0,0,0), mFontLineWidth+1, cv::LINE_AA);
                        cv::putText(src, fieldValues[namePos], cv::Point2d(coordinate.x - (size.width/2), coordinate.y - size.height + baseLine), cv::FONT_ITALIC, fontScale, mColor, mFontLineWidth, cv::LINE_AA);
                    }
                }
            }
        }
    }
}

bool GIS::ShapeRenderer::equidistantCheck(float latitude, float longitude, float centerLatitude, float centerLongitude)
{
    //Degree To radian
    latitude = M_PI * latitude / 180.0f;
    longitude = M_PI * longitude / 180.0f;
    centerLatitude= M_PI * centerLatitude / 180.0f;
    centerLongitude= M_PI * centerLongitude / 180.0f;

    float deltaSigma = std::sin(centerLatitude) * std::sin(latitude) + std::cos(latitude) * std::cos(longitude - centerLongitude);
    if (deltaSigma < 0.0)
    {
        return false;
    }

    return true;
}
