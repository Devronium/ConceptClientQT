#ifndef __AUDIOOBJECT_H
#define __AUDIOOBJECT_H

#include <stdarg.h>
#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#endif

#include "AnsiString.h"
#include "semhh.h"

#include <unistd.h>
#include <stdio.h>
#ifndef NO_SPEEX
#include <speex/speex.h>
#endif
#ifndef NO_OPUS
#include <opus.h>
#endif
#include "AES/AES.h"
#include "BasicMessages.h"

#include <QAudioInput>
#include <QAudioOutput>
#include "ConceptClient.h"


#define P_BITRATE            3725
#define P_BANDWIDTH          1009
#define P_ADDBUFFER          1003
#define P_ADDBUFFER2         1007
#define P_CLEAR              1008

#define SAMPLE               short

int AesRoundSize(int iSize, int iAlignSize);

class AudioObject;

class AudioInfo : public QIODevice {
    Q_OBJECT
    AudioObject *owner;
public:
    AudioInfo(AudioObject *owner, const QAudioFormat &format, QObject *parent);
    ~AudioInfo();
    
    void start(bool recording);
    void stop();
    
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
};


class AudioObject {
public:
    QObject    *stream;
    AudioInfo  *audioInfo;
    int        stream_type;
    int        err;
    int        SAMPLE_RATE;
    int        NUM_CHANNELS;
    int        finished;
    int        quality;
    int        speex_frame_size;
    bool       on_buffer_full;
#ifndef NO_SPEEX
    SpeexBits  bits;
#endif
    void       *enc_state;
    void       *dec_state;
    AnsiString audio_buf;

    SAMPLE     propagation[0xFFF];
    int        propagation_size;
    int        use_compression;
    int        bitrate;
    int        bandwidth;
    bool       quality_changed;
    HHSEM      lock;
    int        maxbuffers;
    int        framesize;
    char       DRM;
    AnsiString key;
    
#ifndef NO_OPUS
    OpusDecoder *dec;
    OpusEncoder *enc;
    opus_int16 *out;
#endif
    
    AES         *encrypt;
    AES         *decrypt;
    AnsiString  ID;
    CConceptClient *owner;
    
    AudioObject(CConceptClient *owner, long ID);
    ~AudioObject();
    
    void InitEncryption();
    void Encrypt(char *buf, int len);
    void Decrypt(AnsiString *result, char *buf, int len);
    void InitDecryption();
    bool IsActive();
    bool Stop();
#ifndef NO_SPEEX
    void SpeexDeBuffer(char *buf, int buf_size, int use_int_instead_of_char = 0);
    int SpeexBuffer2(const SAMPLE *buf, int buf_size);
#endif
#ifndef NO_OPUS
    void OpusDeBuffer(char *buf, int buf_size, int use_int_instead_of_char = 0);
    int OpusBuffer2(const SAMPLE *buf, int buf_size);
#endif
    
    bool Record(int device = -1);
    bool Play(int device = -1);
    
    void ClearBuffer();
    void AddBuffer(void *str_buffer, int buf_len, int _use_int = 0);
};
#endif
