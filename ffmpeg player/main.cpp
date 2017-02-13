#include <stdio.h>
#include <cv.h>
#include <highgui.h>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
};

int main(int argc, char* argv[])
{
	av_register_all();
	avformat_network_init();
	AVFormatContext *pFormatCtx = avformat_alloc_context();

	if(avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0)
	{
		printf("Couldn't open input stream.\n");
		return -1;
	}
	if(avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}
	int videoindex = -1;
	for (int i = 0; i < pFormatCtx->nb_streams; ++i)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
	}
	if (videoindex == -1)
	{
		printf("Didn't find a video stream.\n");
		return -1;
	}
	AVCodecContext *pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("Could not open codec.\n");
		return -1;
	}
	
	AVFrame *pFrame = av_frame_alloc();
	AVFrame *pFrameYUV = av_frame_alloc();
	AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));

	printf("------------- File Information ------------------\n");
	av_dump_format(pFormatCtx,0,argv[1],0);
	printf("-------------------------------------------------\n");

	//struct SwsContext *img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL); 
	int iCr = 0;
	double gtime = 0.0;
	double dStartTime = cvGetTickCount();
	int got_picture = 0;
	cvNamedWindow("Disp", CV_WINDOW_NORMAL);

	while (av_read_frame(pFormatCtx, packet) >= 0)
	{
		if (packet->stream_index == videoindex)
		{
			//Decode
			if (avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet) < 0)
			{
				printf("Decode Error.\n");
				return -1;
			}
			if (got_picture)
			{
				//pFrameYUV->data[0] = buffer;
				//pFrameYUV->data[1] = buffer+pCodecCtx->width*pCodecCtx->height;
				//pFrameYUV->data[2] = buffer+pCodecCtx->width*pCodecCtx->height * 5 / 4;     
				//pFrameYUV->linesize[0]=pCodecCtx->width;
				//pFrameYUV->linesize[1]=pCodecCtx->width/2;   
				//pFrameYUV->linesize[2]=pCodecCtx->width/2;
				//sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, 
				//	pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

				++iCr;
				double dTemptime = cvGetTickCount() - dStartTime;
				if (iCr % 100 == 0)
				{
					printf("frame = %d time = %gms\n", iCr, dTemptime/(cvGetTickFrequency()*1000.0));
				}

				IplImage *dispImage = cvCreateImage(cvSize(pCodecCtx->width, pCodecCtx->height), IPL_DEPTH_8U, 3);
				for (int i = 0; i < pCodecCtx->height; ++i)
				{
					for (int j = 0; j < pCodecCtx->width; ++j)
					{
						// Method 1
						//int y = (pFrame->data[0][i * pFrame->linesize[0] + j] - 16) * 74;
						//int u = pFrame->data[1][(i >> 1) * pFrame->linesize[1] + (j >> 1)] - 128;
						//int v = pFrame->data[2][(i >> 1) * pFrame->linesize[2] + (j >> 1)] - 128;
						//int r = (y + v * 102) >> 6;
						//int g = (y - u * 25 - v * 52) >> 6;
						//int b = (y + u * 127) >> 6;

						// Method 2
						int y = pFrame->data[0][i * pFrame->linesize[0] + j];
						int v = pFrame->data[2][(i >> 1) * pFrame->linesize[2] + (j >> 1)];
						int u = pFrame->data[1][(i >> 1) * pFrame->linesize[1] + (j >> 1)];
						//int r = y + 1.370705*(v - 128);
						//int g = y - 0.698001*(u - 128) - 0.703125*(v - 128);
						//int b = y + 1.732446*(u - 128);
						int r = y + 1.403*(v - 128);
						int g = y - 0.344*(u - 128) - 0.714*(v - 128);
						int b = y + 1.773*(u - 128);

						if (r < 0) { r = 0; }
						if (r > 255) { r = 255; }
						if (g < 0) { g = 0; }
						if (g > 255) { g = 255; }
						if (b < 0) { b = 0; }
						if (b > 255) { b = 255; }
						dispImage->imageData[i*dispImage->widthStep + j * 3] = b;
						dispImage->imageData[i*dispImage->widthStep + j * 3 + 1] = g;
						dispImage->imageData[i*dispImage->widthStep + j * 3 + 2] = r;
					}
				}
				cvShowImage("Disp", dispImage);
				cvWaitKey(1);
				cvReleaseImage(&dispImage);
			}
		}
		av_free_packet(packet);
	}

	//FIX: Flush Frames remained in Codec
	while (true)
	{
		if (avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet) < 0)
		{
			break;
		}
		if (!got_picture)
		{
			break;
		}
		//sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
		
		//pFrameYUV->data[0] = buffer;
		//pFrameYUV->data[1] = buffer+pCodecCtx->width*pCodecCtx->height;
		//pFrameYUV->data[2] = buffer+pCodecCtx->width*pCodecCtx->height * 5 / 4;     
		//pFrameYUV->linesize[0]=pCodecCtx->width;
		//pFrameYUV->linesize[1]=pCodecCtx->width/2;   
		//pFrameYUV->linesize[2]=pCodecCtx->width/2;
		//sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, 
		//	pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
		
		//IplImage *dispImage = cvCreateImage(cvSize(pCodecCtx->width, pCodecCtx->height), IPL_DEPTH_8U, 3);
		//for (int i = 0; i < pCodecCtx->height; ++i)
		//{
		//	for (int j = 0; j < pCodecCtx->width; ++j)
		//	{
		//		// Method 1
		//		int y = (pFrameYUV->data[0][i * pCodecCtx->width + j] - 16) * 74;
		//		int v = pFrameYUV->data[2][(i >> 1) * (pCodecCtx->width >> 1) + (j >> 1)] - 128;
		//		int u = pFrameYUV->data[1][(i >> 1) * (pCodecCtx->width >> 1) + (j >> 1)] - 128;
		//		int r = (y + v * 102) >> 6;
		//		int g = (y - u * 25 - v * 52) >> 6;
		//		int b = (y + u * 127) >> 6;

		//		// Method 2
		//		//int y = pData.Y[i * pInfo.Width + j];
		//		//int v = pData.V[(i >> 1) * pInfo.Width + ((j >> 1) << 1)];
		//		//int u = pData.U[(i >> 1) * pInfo.Width + ((j >> 1) << 1)];
		//		////int r = y + 1.370705*(v - 128);
		//		////int g = y - 0.698001*(u - 128) - 0.703125*(v - 128);
		//		////int b = y + 1.732446*(u - 128);
		//		//int r = y + 1.403*(v - 128);
		//		//int g = y - 0.344*(u - 128) - 0.714*(v - 128);
		//		//int b = y + 1.773*(u - 128);

		//		if (r < 0) { r = 0; }
		//		if (r > 255) { r = 255; }
		//		if (g < 0) { g = 0; }
		//		if (g > 255) { g = 255; }
		//		if (b < 0) { b = 0; }
		//		if (b > 255) { b = 255; }
		//		dispImage->imageData[i*dispImage->widthStep + j * 3] = b;
		//		dispImage->imageData[i*dispImage->widthStep + j * 3 + 1] = g;
		//		dispImage->imageData[i*dispImage->widthStep + j * 3 + 2] = r;
		//	}
		//}
		//cvShowImage("Disp", dispImage);
		//cvWaitKey(1);
		//cvReleaseImage(&dispImage);
	}
	//sws_freeContext(img_convert_ctx);
	av_free(pFrame);
	av_free(pFrameYUV);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
	getchar();
	return 0;
}
