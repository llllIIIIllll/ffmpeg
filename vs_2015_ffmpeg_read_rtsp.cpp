#include <stdio.h>

#define __STDC_CONSTANT_MACROS

extern "C"
{
#include <libavcodec\avcodec.h>
#include <libavformat\avformat.h>
#include <libswscale\swscale.h>
#include <libavutil\imgutils.h>
};

#include <opencv2\core\core.hpp>
#include <opencv2\opencv.hpp>

#pragma warning(disable:4996)

using namespace cv;





int main(int argc, char* argv[])
{
	AVFormatContext   *pFormatCtx;
	int				  i, videoindex;
	AVCodecContext	  *pCodecCtx;
	AVCodec			  *pCodec;
	AVFrame			  *pFrame, *pFrameYUV;
	unsigned char     *out_buffer;
	AVPacket		  *packet;
	int				  y_size;
	int				  ret, got_picture;
	struct SwsContext *img_convert_ctx;

	struct SwsContext *pSwsCtx;
	AVFrame *video_frameRGB = NULL;
	video_frameRGB = av_frame_alloc();
	uint8_t *outBuff = NULL;
	int frameSize;


	//![0] 缓冲区接口大小
	//AVDictionary *dic = NULL;
	//int res = av_dict_set(&dic, "bufsize", "655360", 0);
	
	// tcp
	AVDictionary* options = NULL;
	av_dict_set(&options, "rtsp_transport", "tcp", 0);

	//![0]

	char filePath[] = "rtsp://admin:infecon123@169.254.13.104:554/h264/ch1/main/av_stream";

	FILE *fp_yuv = fopen("output.yuv", "wb+");

	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	//![0]
	if (avformat_open_input(&pFormatCtx, filePath, NULL, &options) != 0)
	{
		printf("Couldn't open input stream.\n");
		return -1;
	}
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}
	videoindex = -1;
	for (i = 0; i < pFormatCtx->nb_streams; ++i)
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

	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
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

	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
		AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);

	packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	//Output Info-----------------------------
	printf("--------------- File Information ----------------\n");
	av_dump_format(pFormatCtx, 0, filePath, 0);
	printf("-------------------------------------------------\n");

	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);


	cv::namedWindow(filePath, 1);
	while (av_read_frame(pFormatCtx, packet) >= 0) {

		if (packet->stream_index == videoindex) {
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if (ret < 0) {
				printf("Decode Error.\n");
				return -1;
			}

			//![1] 判断丢包
			if (got_picture && packet->nIsLostPackets == 0 ){
				
				{
					frameSize = avpicture_get_size(AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);
					outBuff = (uint8_t*)av_malloc(frameSize);
					avpicture_fill((AVPicture*)video_frameRGB, outBuff, AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);

					pSwsCtx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
						pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_BGR24,
						SWS_BICUBIC, NULL, NULL, NULL);
				}

				cv::Mat outMat = cv::Mat::zeros(pCodecCtx->height, pCodecCtx->width, CV_8UC3);
				sws_scale(pSwsCtx, pFrame->data,
					pFrame->linesize, 0, pCodecCtx->height,
					video_frameRGB->data, video_frameRGB->linesize);

				memcpy(outMat.data, outBuff, frameSize);

				cv::imshow(filePath, outMat);
				cv::waitKey(18);
				/*sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
					pFrameYUV->data, pFrameYUV->linesize);
				y_size = pCodecCtx->width*pCodecCtx->height;*/

				//fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y 
				//fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
				//fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V

				// change to opencv2 mat 
				printf("Succeed to decode 1 frame!\n");

			} 
			else {

				waitKey(18);
			}
		}
		av_free_packet(packet);
	}

	//! [2] write to YUV
	//flush decoder
	//FIX: Flush Frames remained in Codec
	//while (1) {
	//	ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
	//	if (ret < 0)
	//		break;
	//	if (!got_picture)
	//		break;
	//	sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
	//		pFrameYUV->data, pFrameYUV->linesize);

	//	int y_size = pCodecCtx->width*pCodecCtx->height;

	//	fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y 
	//	fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
	//	fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V

	//	printf("Flush Decoder: Succeed to decode 1 frame!\n");
	//}

	sws_freeContext(img_convert_ctx);

	fclose(fp_yuv);

	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;

}
    
