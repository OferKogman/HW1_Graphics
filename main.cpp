#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <iostream>
#include <cmath>
#include <string>

int processImage(const char* fileName) {
    std::string baseName = fileName;
    size_t dot = baseName.rfind('.');
    if (dot != std::string::npos) {
        baseName = baseName.substr(0, dot);
    }

    std::string grayName = baseName + "_Grayscale.png";
    std::string halftoneName = baseName + "_Haftone.png";
    std::string floydName = baseName + "_FloyedSteinberg.png";
    std::string cannyName = baseName + "_Canny.png";

    int width, height, channels;

    unsigned char* rgba = stbi_load(fileName, &width, &height, &channels, 4);
    if (rgba == nullptr) {
        std::cout << "Failed: " << stbi_failure_reason() << std::endl;
        return 1;
    }

    int pixelCount = width * height;
    unsigned char* gray = new unsigned char[pixelCount];

    for (int i = 0; i < pixelCount; i++) {
        unsigned char r = rgba[i * 4 + 0];
        unsigned char g = rgba[i * 4 + 1];
        unsigned char b = rgba[i * 4 + 2];

        double value = 0.299 * r + 0.587 * g + 0.114 * b;
        gray[i] = (unsigned char)value;
    }

    if (stbi_write_png(grayName.c_str(), width, height, 1, gray, width) == 0) {
        std::cout << "Failed to write " << grayName << std::endl;
    } else {
        std::cout << "Saved " << grayName << std::endl;
    }

    int outWidth = width * 2;
    int outHeight = height * 2;
    unsigned char* halftone = new unsigned char[outWidth * outHeight];

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int level = gray[y * width + x] / 52;

            unsigned char tl, tr, bl, br;

            if (level == 0) {
                tl = 0;   tr = 0;
                bl = 0;   br = 0;
            } else if (level == 1) {
                tl = 0;   tr = 0;
                bl = 0;   br = 255;
            } else if (level == 2) {
                tl = 0;   tr = 255;
                bl = 255; br = 0;
            } else if (level == 3) {
                tl = 0;   tr = 255;
                bl = 255; br = 255;
            } else {
                tl = 255; tr = 255;
                bl = 255; br = 255;
            }

            int topRow = 2 * y;
            int bottomRow = 2 * y + 1;
            int leftCol = 2 * x;
            int rightCol = 2 * x + 1;

            halftone[topRow * outWidth + leftCol] = tl;
            halftone[topRow * outWidth + rightCol] = tr;
            halftone[bottomRow * outWidth + leftCol] = bl;
            halftone[bottomRow * outWidth + rightCol] = br;
        }
    }

    if (stbi_write_png(halftoneName.c_str(), outWidth, outHeight, 1, halftone, outWidth) == 0) {
        std::cout << "Failed to write " << halftoneName << std::endl;
    } else {
        std::cout << "Saved " << halftoneName << std::endl;
    }

    double* fs = new double[pixelCount];
    for (int i = 0; i < pixelCount; i++) {
        fs[i] = gray[i];
    }

    unsigned char* floyd = new unsigned char[pixelCount];

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;

            double oldValue = fs[index];
            if (oldValue < 0) {
                oldValue = 0;
            }
            if (oldValue > 255) {
                oldValue = 255;
            }

            int level = (int)(oldValue / 17 + 0.5);
            double newValue = level * 17;
            floyd[index] = (unsigned char)newValue;

            double error = oldValue - newValue;

            bool hasLeft = x - 1 >= 0;
            bool hasRight = x + 1 < width;
            bool hasBottom = y + 1 < height;

            if (hasLeft && hasRight && hasBottom) {
                fs[index + 1] += error * 7 / 16;
                fs[index + width - 1] += error * 3 / 16;
                fs[index + width] += error * 5 / 16;
                fs[index + width + 1] += error * 1 / 16;
            } else {
                int count = 0;
                if (hasRight) {
                    count++;
                }
                if (hasBottom && hasLeft) {
                    count++;
                }
                if (hasBottom) {
                    count++;
                }
                if (hasBottom && hasRight) {
                    count++;
                }

                if (count > 0) {
                    double share = error / count;
                    if (hasRight) {
                        fs[index + 1] += share;
                    }
                    if (hasBottom && hasLeft) {
                        fs[index + width - 1] += share;
                    }
                    if (hasBottom) {
                        fs[index + width] += share;
                    }
                    if (hasBottom && hasRight) {
                        fs[index + width + 1] += share;
                    }
                }
            }
        }
    }

    if (stbi_write_png(floydName.c_str(), width, height, 1, floyd, width) == 0) {
        std::cout << "Failed to write " << floydName << std::endl;
    } else {
        std::cout << "Saved " << floydName << std::endl;
    }

    int gk[5][5] = {
        {2, 4, 5, 4, 2},
        {4, 9, 12, 9, 4},
        {5, 12, 15, 12, 5},
        {4, 9, 12, 9, 4},
        {2, 4, 5, 4, 2}
    };

    double* blur = new double[pixelCount];

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;
            if (y < 2 || y >= height - 2 || x < 2 || x >= width - 2) {
                blur[index] = gray[index];
            } else {
                double sum = 0;
                for (int ky = -2; ky <= 2; ky++) {
                    for (int kx = -2; kx <= 2; kx++) {
                        sum += gk[ky + 2][kx + 2] * gray[(y + ky) * width + (x + kx)];
                    }
                }
                blur[index] = sum / 159;
            }
        }
    }

    double* magnitude = new double[pixelCount];
    double* angle = new double[pixelCount];

    for (int i = 0; i < pixelCount; i++) {
        magnitude[i] = 0;
        angle[i] = 0;
    }

    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            double topLeft = blur[(y - 1) * width + (x - 1)];
            double top = blur[(y - 1) * width + x];
            double topRight = blur[(y - 1) * width + (x + 1)];
            double left = blur[y * width + (x - 1)];
            double right = blur[y * width + (x + 1)];
            double bottomLeft = blur[(y + 1) * width + (x - 1)];
            double bottom = blur[(y + 1) * width + x];
            double bottomRight = blur[(y + 1) * width + (x + 1)];

            double gx = (topRight + 2 * right + bottomRight) - (topLeft + 2 * left + bottomLeft);
            double gy = (bottomLeft + 2 * bottom + bottomRight) - (topLeft + 2 * top + topRight);

            int index = y * width + x;
            magnitude[index] = sqrt(gx * gx + gy * gy);

            double a = atan2(gy, gx) * 180.0 / 3.14159265358979;
            if (a < 0) {
                a += 180;
            }
            angle[index] = a;
        }
    }

    double* thin = new double[pixelCount];

    for (int i = 0; i < pixelCount; i++) {
        thin[i] = 0;
    }

    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int index = y * width + x;
            double a = angle[index];
            double n1, n2;

            if (a < 22.5 || a >= 157.5) {
                n1 = magnitude[index - 1];
                n2 = magnitude[index + 1];
            } else if (a < 67.5) {
                n1 = magnitude[index - width - 1];
                n2 = magnitude[index + width + 1];
            } else if (a < 112.5) {
                n1 = magnitude[index - width];
                n2 = magnitude[index + width];
            } else {
                n1 = magnitude[index - width + 1];
                n2 = magnitude[index + width - 1];
            }

            if (magnitude[index] >= n1 && magnitude[index] >= n2) {
                thin[index] = magnitude[index];
            }
        }
    }

    double maxValue = 0;
    for (int i = 0; i < pixelCount; i++) {
        if (thin[i] > maxValue) {
            maxValue = thin[i];
        }
    }

    double highThreshold = maxValue * 0.15;
    double lowThreshold = maxValue * 0.05;

    unsigned char* canny = new unsigned char[pixelCount];

    for (int i = 0; i < pixelCount; i++) {
        if (thin[i] >= highThreshold) {
            canny[i] = 255;
        } else if (thin[i] >= lowThreshold) {
            canny[i] = 75;
        } else {
            canny[i] = 0;
        }
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (int y = 1; y < height - 1; y++) {
            for (int x = 1; x < width - 1; x++) {
                int index = y * width + x;
                if (canny[index] == 75) {
                    bool touchesStrong = false;
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            if (canny[(y + dy) * width + (x + dx)] == 255) {
                                touchesStrong = true;
                            }
                        }
                    }
                    if (touchesStrong) {
                        canny[index] = 255;
                        changed = true;
                    }
                }
            }
        }
    }

    for (int i = 0; i < pixelCount; i++) {
        if (canny[i] == 75) {
            canny[i] = 0;
        }
    }

    if (stbi_write_png(cannyName.c_str(), width, height, 1, canny, width) == 0) {
        std::cout << "Failed to write " << cannyName << std::endl;
    } else {
        std::cout << "Saved " << cannyName << std::endl;
    }

    stbi_image_free(rgba);
    delete[] gray;
    delete[] halftone;
    delete[] fs;
    delete[] floyd;
    delete[] blur;
    delete[] magnitude;
    delete[] angle;
    delete[] thin;
    delete[] canny;
    return 0;
}

int main() {
    processImage("Ash.png");
    processImage("Lenna.png");
    return 0;
}