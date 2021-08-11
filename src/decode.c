#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>

static int check_decoder_return(int ret)
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

static char * h264_decode(void * data, size_t data_size, size_t * output_size)
{
  AVPacket * pkt;
  AVFrame * frame;
  int ret, should_break;
  const AVCodec * codec;
  char * output, * output_ptr;
  AVCodecParserContext * parser;
  AVCodecContext * codec_context = NULL;

  /* The beginning of the data has the output size*/
  memcpy(output_size, data, sizeof(size_t));
  data += sizeof(size_t);
  data_size -= sizeof(size_t);

  output = malloc(* output_size);
  /* The world of pointer arithmetic, Oh so sweet */
  output_ptr = output;
  codec = avcodec_find_decoder(AV_CODEC_ID_H264);
  if(!codec)
  {
    fprintf(stderr, "Cannot find H264 codec\n");
    exit(1);
  }

  codec_context = avcodec_alloc_context3(codec);
  if (!codec_context)
  {
    fprintf(stderr, "Could not allocate video codec context\n");
    exit(1);
  }

  if(codec->capabilities & AV_CODEC_CAP_TRUNCATED)
  {
    codec_context->flags |= AV_CODEC_FLAG_TRUNCATED;
  }

  ret = avcodec_open2(codec_context, codec, NULL);
  if(ret < 0)
  {
    fprintf(stderr, "Could not open codec\n");
    exit(1);
  }

  parser = av_parser_init(AV_CODEC_ID_H264);
  if(!parser)
  {
    fprintf(stderr,"Cannot create H264 parser.\n");
    exit(1);
  }

  frame = av_frame_alloc();
  if(!frame)
  {
    fprintf(stderr, "Could not allocate frame\n");
    exit(1);
  }

  pkt = av_packet_alloc();
  if(!pkt)
  {
    fprintf(stderr, "Could not allocate packet buffer\n");
    exit(1);
  }

  while(data_size > 0)
  {
    /* Parse stream & get required length */
    ret = av_parser_parse2(parser, codec_context, &pkt->data, &pkt->size, data, data_size, 0, 0, AV_NOPTS_VALUE);
    if(ret < 0)
    {
      fprintf(stderr, "Error while parsing\n");
      exit(1);
    }

    /* Shift data */
    data += pkt->size;
    data_size  -= pkt->size;

    if(pkt->size)
    {
      ret = avcodec_send_packet(codec_context, pkt);
      should_break = check_decoder_return(ret);
    }

    ret = avcodec_receive_frame(codec_context, frame);
    should_break = check_decoder_return(ret);
    if(should_break)
    {
      continue;
    }

    /* data -> output buffer */
    for (int y = 0; y < frame->height; y++)
    {
      memcpy(output_ptr, (char *) &frame->data[0][y * frame->linesize[0]], frame->width);
      output_ptr += frame->width;
    }
  }

  /* flush the decoder */
  ret = avcodec_send_packet(codec_context, NULL);
  while(1)
  {
    ret = avcodec_receive_frame(codec_context, frame);
    should_break = check_decoder_return(ret);
    if(should_break)
    {
      break;
    }
    for (int y = 0; y < frame->height; y++)
    {
      memcpy(output_ptr, (char *) &frame->data[0][y * frame->linesize[0]], frame->width);
      output_ptr += frame->width;
    }
  }

  /* dealloc heap variables */
  av_parser_close(parser);
  avcodec_free_context(&codec_context);
  av_frame_free(&frame);
  av_packet_free(&pkt);

  return output;
}
