//
// Created by Administrator on 2017/12/10.
//

#ifndef FASTPLAYER_H264DECODEDEFINE_H
#define FASTPLAYER_H264DECODEDEFINE_H
typedef struct H264FrameDef{
    unsigned int length;
    unsigned char* dataBufer;
}H264Frame;

typedef struct H264YUVDef{

    unsigned int    width;
    unsigned int    height;
    H264Frame       yData;
    H264Frame       uData;
    H264Frame       vData;


}H264YUV_Frame;






#endif //FASTPLAYER_H264DECODEDEFINE_H
