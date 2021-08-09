#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>

static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, int pkt_idx)
{
  int ret = avcodec_send_frame(enc_ctx, frame);
  if(ret < 0)
  {
    fprintf(stderr, "Error sending a frame for encoding\n");
    exit(1);
  }

  while(ret >= 0)
  {
    ret = avcodec_receive_packet(enc_ctx, &pkt[pkt_idx]);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
      return;
    else if(ret < 0)
    {
      fprintf(stderr, "Error during encoding\n");
      exit(1);
    }
  }
}

static char * h264_encode(char * data, size_t size, size_t height, size_t width, int item_size, size_t * output_size)
{
  const AVCodec * codec;
  AVCodecContext * codec_context = NULL;
  AVDictionary * param = 0;
  int ret, nframes, pkt_idx;
  AVFrame * frame;
  AVPacket * pkt;

  av_log_set_level(AV_LOG_WARNING);
  // avcodec_register_all();
  // int got_output;
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
  
  ret = av_image_alloc(frame->data, frame->linesize, codec_context->width, codec_context->height, codec_context->pix_fmt, 32);
  if(ret < 0)
  {
    fprintf(stderr, "Could not allocate raw picture buffer\n");
    exit(1);
  }

  /* Zero Cb and Cr */
  memset(frame->data[1], 0, codec_context->height*frame->linesize[1]/2);
  memset(frame->data[2], 0, codec_context->height*frame->linesize[2]/2);

  nframes = size/(width*height);
  pkt = malloc(sizeof(AVPacket)*(nframes + 1));
  pkt_idx = 0;
  for(int i = 0; i < nframes+1; i++)
  {
    av_init_packet(&pkt[i]);
    // packet data will be allocated by the encoder
    pkt[i].data = NULL;
    pkt[i].size = 0;
  }

  for(int i = 0; i < nframes; i++)
  {
    // ret = av_frame_make_writable(frame);
    // if(ret < 0)
    // {
    //   fprintf(stderr, "Could not write to buffer\n");
    //   exit(1);
    // }
    /* Y */
    size_t idx = 0;
    for(int y = 0; y < height; y++)
    { 
      memcpy(&frame->data[0][y * frame->linesize[0]], data, width);
      data += width;
    } 
  
    /* Skip Cb and Cr */
    frame->pts = i;
    encode(codec_context, frame, pkt, pkt_idx);
    pkt_idx++;

    // int ret_val_send = avcodec_send_frame(codec_context, frame);
    // if(ret_val_send < 0)
    // {
    //   fprintf(stderr, "Error encoding frame\n");
    //   return NULL;
    // }
    // else
    // {
    //   int ret_val_received = avcodec_receive_packet(codec_context, &pkt[pkt_idx]);
    //   if(ret_val_received < 0)
    //   {
    //     fprintf(stderr, "%d", ret_val_received);
    //     fprintf(stderr, "Error receiving encoded frame\n");
    //     // return NULL;
    //   }
    //   else
    //   {
    //     pkt_idx++;
    //   }
    // }
  }

  /* get the delayed frames */
  /* flush the encoder */
  // for(int i = 0; i < nframes; i++)
  // {
  encode(codec_context, NULL, pkt, 0);
  // }
  // for(got_output = 1; got_output; pkt_idx++)
  // {
  //   int ret_val_send = avcodec_send_frame(codec_context, NULL);
  //   if(ret_val_send < 0)
  //   {
  //     fprintf(stderr, "Error encoding frame\n");
  //     return NULL;
  //   }
  //   else
  //   {
  //     int ret_val_received = avcodec_receive_packet(codec_context, &pkt[pkt_idx]);
  //     if(ret_val_received < 0)
  //     {
  //       fprintf(stderr, "Error receiving encoded frame\n");
  //       return NULL;
  //     }
  //   }
  // }

  /* Calculate output size*/
  /* We'll save the input size in the first 8 bytes */
  *output_size = sizeof(size);
  for(int i = 0; i < nframes; i++)
  {
    *output_size += pkt[i].size;
  }

  /* Allocate output */
  char * output = malloc(*output_size);
  char * ptr = output;
  memcpy(ptr, &size, sizeof(size));
  ptr += sizeof(size);
  /* Copy data to output and free packets */
  for(int i = 0; i < nframes; i++)
  {
    memcpy(ptr, pkt[i].data, pkt[i].size);
    ptr += pkt[i].size;
    av_packet_unref(&pkt[i]);
  }
  
  avcodec_free_context(&codec_context);
  av_frame_free(&frame);
  free(pkt);
  
  return output;
}
