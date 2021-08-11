#include <libavutil/opt.h>
#include <libavutil/common.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/mathematics.h>
#include <libavutil/channel_layout.h>

static int check_encoder_return(int ret)
{
  if(ret == 0)
  {
    return 0;
  }
  else
  {
    if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
    {
      return 1;
    }
    else if(ret == AVERROR(EINVAL))
    {
      fprintf(stderr, "Codec internal errors\n");
      exit(1);
    }
    else if(ret == AVERROR(ENOMEM))
    {
      fprintf(stderr, "Failed to add packet to internal queue\n");
      exit(1);
    }
    else
    {
      fprintf(stderr, "Unknown encoder errors\n");
      exit(1);
    }
  }
}

static char * h264_encode(char * data, size_t size, size_t height, size_t width, int item_size, size_t * output_size)
{
  AVFrame * frame;
  const AVCodec * codec;
  AVDictionary * param = 0;
  char * output, * output_ptr;
  AVCodecContext * codec_context = NULL;
  int ret, n_frames, frm_idx, pkt_idx, should_break;

  av_log_set_level(AV_LOG_WARNING);
  uint8_t endcode[] = { 0, 0, 1, 0xb7 };

  codec = avcodec_find_encoder(AV_CODEC_ID_H264);
  if(!codec) {
    fprintf(stderr, "Codec '%s' not found\n", "H264");
    exit(1);
  }

  codec_context = avcodec_alloc_context3(codec);
  if(!codec_context) {
    fprintf(stderr, "Could not allocate video codec context\n");
    exit(1);
  }

  if(item_size == 1)
  {
    /* Nothing to do */
  }
  else if(item_size == 2)
  {
    width *= 2;
  }
  else if(item_size == 4)
  {
    width *= 2;
    height *= 2;
  }
  else
  {
    fprintf(stderr, "Item size must be 1, 2 or 4 bytes\n");
    exit(1);
  }

  /* resolution must be a multiple of two */
  if((height % 2) || (width % 2))
  {
    fprintf(stderr, "Height and width must be a multiple of two\n");
    exit(1);    
  }

  /* put sample parameters */
  codec_context->width = width;
  codec_context->height = height;
  codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
  codec_context->time_base = (AVRational){1, 30};
  codec_context->framerate = (AVRational){30, 1};

  /* Set the medium preset and qp 0 for lossless encoding */
  av_dict_set(&param, "qp", "0", 0);
  av_dict_set(&param, "preset", "medium", 0);

  /* open codec */
  ret = avcodec_open2(codec_context, codec, &param);
  if(ret < 0)
  {
    fprintf(stderr, "Could not open codec\n");
    exit(1);
  }

  frame = av_frame_alloc();
  if(!frame)
  {
    fprintf(stderr, "Could not allocate video frame\n");
    exit(1);
  }

  frame->format = codec_context->pix_fmt;
  frame->width  = codec_context->width;
  frame->height = codec_context->height;

  ret = av_frame_get_buffer(frame, 0);
  if(ret < 0)
  {
    fprintf(stderr, "Could not allocate the video frame data\n");
    exit(1);
  }

  /* Zero Cb and Cr */
  memset(frame->data[1], 0, codec_context->height*frame->linesize[1]/2);
  memset(frame->data[2], 0, codec_context->height*frame->linesize[2]/2);

  pkt_idx = 0;
  frm_idx = 0;
  n_frames = size/(width*height);
  AVPacket * pkt[n_frames + 1];
  /* We'll save the input size in the first 8 bytes */
  * output_size = sizeof(size);

  while(frm_idx < n_frames)
  {
    /* Y */
    for(int y = 0; y < height; y++)
    {
      memcpy(&frame->data[0][y * frame->linesize[0]], data, width);
      data += width;
    }
    /* Skip Cb and Cr */
    frame->pts = frm_idx;
    ret = avcodec_send_frame(codec_context, frame);
    frm_idx++;
    should_break = check_encoder_return(ret);

    pkt[pkt_idx] = av_packet_alloc();
    ret = avcodec_receive_packet(codec_context, pkt[pkt_idx]);
    should_break = check_encoder_return(ret);
    if(should_break)
    {
      continue;
    }

    /* Calculate output size*/
    * output_size += pkt[pkt_idx]->size;
    pkt_idx++;
  }

  /* get the delayed frames */
  /* flush the encoder */
  ret = avcodec_send_frame(codec_context, NULL);
  while(1)
  {
    pkt[pkt_idx] = av_packet_alloc();
    ret = avcodec_receive_packet(codec_context, pkt[pkt_idx]);
    should_break = check_encoder_return(ret);
    if(should_break)
    {
      break;
    }
    * output_size += pkt[pkt_idx]->size;
    pkt_idx++;
  }

  /* Allocate output */
  output = av_mallocz(* output_size);
  /* The world of pointer arithmetic, Oh so sweet */
  output_ptr = output;
  memcpy(output_ptr, &size, sizeof(size));
  output_ptr += sizeof(size);

  /* Copy data to output and free packets */
  for(int i = 0; i <= n_frames; i++)
  {
    memcpy(output_ptr, (char *) pkt[i]->data, pkt[i]->size);
    output_ptr += pkt[i]->size;
    av_packet_free(&pkt[i]);
  }
  
  avcodec_free_context(&codec_context);
  av_frame_free(&frame);
  
  return output;
}
