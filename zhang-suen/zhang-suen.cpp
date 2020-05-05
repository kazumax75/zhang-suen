// zhang-suen.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include <opencv2/opencv.hpp>

using namespace cv;

int setBuf(char *buf, int x, int y, int mx, int d)
{
	buf[x + 1 + (y + 1)*mx] = d;
	return 0;
}

int getBuf(char *buf, int x, int y, int mx)
{
	return buf[(x + 1) + (y + 1)*mx];
}


int getConnect(int *sn)
{
	int sum;
	int i, j, k;

	sum = 0;
	for (i = 1; i<9; i += 2) {
		j = i + 1;
		k = i + 2;
		if (j>8) j -= 8;
		if (k>8) k -= 8;
		sum += (sn[i] - sn[i] * sn[j] * sn[k]);
	}
	return sum;
}

int delPoint(char *buf, int x, int y, int mx)
{
	int n[9], sn[9];
	int i, j, k;
	int sum, psum;

	n[1] = getBuf(buf, x + 1, y, mx);
	n[2] = getBuf(buf, x + 1, y - 1, mx);
	n[3] = getBuf(buf, x, y - 1, mx);
	n[4] = getBuf(buf, x - 1, y - 1, mx);
	n[5] = getBuf(buf, x - 1, y, mx);
	n[6] = getBuf(buf, x - 1, y + 1, mx);
	n[7] = getBuf(buf, x, y + 1, mx);
	n[8] = getBuf(buf, x + 1, y + 1, mx);
	for (i = 1; i<9; i++) {
		if (n[i]<0) sn[i] = -n[i];
		else sn[i] = n[i];
	}
	/* 境界であるか */
	if (n[1] + n[3] + n[5] + n[7] == 4) return false;

	sum = psum = 0;
	for (i = 1; i<9; i++) {
		psum += sn[i];
		if (n[i]>0)
			sum += n[i];
	}
	/* 端点か */
	if (psum<2) return false;

	/* 孤立点か */
	if (sum<1) return false;

	/* 連結性を保持できるか */
	sum = getConnect(sn);
	if (sum != 1) return false;

	/* 連結性を保持できるか2 */
	for (i = 1; i<9; i++) {
		int tmp;

		if (n[i]<0) sn[i] = 0;
	}
	sum = getConnect(sn);
	if (sum != 1) return false;
	return true;
}


void thinningIte(Mat& img, int pattern) {

	Mat del_marker = Mat::ones(img.size(), CV_8UC1);
	int x, y;

	for (y = 1; y < img.rows - 1; ++y) {

		for (x = 1; x < img.cols - 1; ++x) {

			int v9, v2, v3;
			int v8, v1, v4;
			int v7, v6, v5;

			v1 = img.data[y   * img.step + x   * img.elemSize()];
			v2 = img.data[(y - 1) * img.step + x   * img.elemSize()];
			v3 = img.data[(y - 1) * img.step + (x + 1) * img.elemSize()];
			v4 = img.data[y   * img.step + (x + 1) * img.elemSize()];
			v5 = img.data[(y + 1) * img.step + (x + 1) * img.elemSize()];
			v6 = img.data[(y + 1) * img.step + x   * img.elemSize()];
			v7 = img.data[(y + 1) * img.step + (x - 1) * img.elemSize()];
			v8 = img.data[y   * img.step + (x - 1) * img.elemSize()];
			v9 = img.data[(y - 1) * img.step + (x - 1) * img.elemSize()];

			int S = (v2 == 0 && v3 == 1) + (v3 == 0 && v4 == 1) +
				(v4 == 0 && v5 == 1) + (v5 == 0 && v6 == 1) +
				(v6 == 0 && v7 == 1) + (v7 == 0 && v8 == 1) +
				(v8 == 0 && v9 == 1) + (v9 == 0 && v2 == 1);

			int N = v2 + v3 + v4 + v5 + v6 + v7 + v8 + v9;

			int m1 = 0, m2 = 0;

			if (pattern == 0) m1 = (v2 * v4 * v6);
			if (pattern == 1) m1 = (v2 * v4 * v8);

			if (pattern == 0) m2 = (v4 * v6 * v8);
			if (pattern == 1) m2 = (v2 * v6 * v8);

			if (S == 1 && (N >= 2 && N <= 6) && m1 == 0 && m2 == 0)
				del_marker.data[y * del_marker.step + x * del_marker.elemSize()] = 0;
		}
	}

	img &= del_marker;
}

void thinning(const Mat& src, Mat& dst) {
	dst = src.clone();
	dst /= 255;         // 0は0 , 1以上は1に変換される

	Mat prev = Mat::zeros(dst.size(), CV_8UC1);
	Mat diff;

	do {
		thinningIte(dst, 0);
		thinningIte(dst, 1);
		absdiff(dst, prev, diff);
		dst.copyTo(prev);
	} while (countNonZero(diff) > 0);

	dst *= 255;
}

bool getPixel(cv::Mat mat, int x, int y, cv::Vec3b& col) {
	if (x < 0 || mat.cols <= x) return false;
	if (y < 0 || mat.rows <= y) return false;

	cv::Vec3b *p = mat.ptr<cv::Vec3b>(y);
	col = p[x];
	return true;
}

void setPixel(cv::Mat mat, int x, int y, cv::Vec3b col) {
	if (x < 0 || mat.cols <= x) return;
	if (y < 0 || mat.rows <= y) return;

	Vec3b *p;
	p = mat.ptr<cv::Vec3b>(y);
	p[x] = col;
}


// エフェクト処理
int effect(cv::Mat& img, cv::Mat& outimg)
{
	int i, m;
	int x, y;
	int xx, yy;
	int mx, my;
	int rmx, rmy;
	int del;
	Vec3b col;
	int val, nval;
	char *img1;
	int x1, y1, x2, y2;

	x1 = 0;
	y1 = 0;
	x2 = img.cols - 1;
	y2 = img.rows - 1;

	rmx = (x2 - x1) + 1;
	rmy = (y2 - y1) + 1;
	/* 距離バッファは周囲１画素分余分にとる */
	mx = rmx + 2;
	my = rmy + 2;

	img1 = (char *)malloc(sizeof(char)*mx*my);
	for (i = 0; i<mx*my; i++) {
		img1[i] = 0;
	}

	for (y = y1; y <= y2; y++) {
		for (x = x1; x <= x2; x++) {
			getPixel(img, x, y, col);	//画像上の画素情報を取得
			if (col[2]>0) {
				setBuf(img1, x - x1, y - y1, mx, 1);
			}
		}
	}
	del = 1;
	while (del>0) {
		del = 0;
		for (y = 0; y<rmy; y++) {
			for (x = 0; x<rmx; x++) {
				val = getBuf(img1, x, y, mx);
				if (val == 1) {
					if (delPoint(img1, x, y, mx)) {
						val = -1;
						del++;
					}
				}
				setBuf(img1, x, y, mx, val);
			}
		}
		for (i = 0; i<mx*my; i++) {
			if (img1[i] == -1)
				img1[i] = 0;
		}
	}
	for (y = y1; y <= y2; y++) {
		for (x = x1; x <= x2; x++) {
			val = getBuf(img1, x - x1, y - y1, mx);
			if (val == 1) {
				col[2] = 255;
				col[1] = 255;
				col[0] = 255;
			}
			else {
				col[2] = 0;
				col[1] = 0;
				col[0] = 0;
			}
			setPixel(outimg, x, y, col);
		}
	}
	return true;
}



int main() {
	Mat img = imread(
"C:\\Users\\kento\\Google ドライブ（moreandhow555@gmail.com）\\Programming\\Python\\resi-to_morufo\\fukidashi.png"
	);

	Mat img2;
	cvtColor(img, img2, COLOR_BGR2GRAY);
	threshold(img2, img, 160, 255, THRESH_BINARY);

	//morphologyEx(img, img2, MORPH_OPEN, getStructuringElement(MORPH_ELLIPSE, Size(1, 1)));


	//zhang-suen
	thinning(img, img2);

	//hildtch
	//effect(img, img2);


	//cv::Mat element4 = (cv::Mat_<uchar>(3, 3) << 0, 1, 0, 1, 1, 1, 0, 1, 0);
	//cv::dilate(img, img2, element4, cv::Point(-1, -1), 2);

	imshow("src", img);
	imshow("dst", img2);
	waitKey();
	return 0;
}

