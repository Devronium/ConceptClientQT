//------------ standard header -----------------------------------//
#include "stdlibrary.h"
//------------ end of standard header ----------------------------//
#include <stdarg.h>
#ifdef _WIN32
 #include <windows.h>
 #include <fcntl.h>
//#define pipe(phandles)	_pipe (phandles, 4096, _O_BINARY)
#endif

#include "AnsiString.h"
#include "semhh.h"

#include <portaudio.h>
#include <unistd.h>
#include <stdio.h>
#include <speex/speex.h>
#ifdef _WIN32
 #include <opus/opus.h>
#else
 #include <opus.h>
#endif
#include "AES/aes.h"

#define CUSTOM_CONTROL_ID    1016

#define P_SAMPLERATE         1000
#define P_NUMCHANNELS        1001
#define P_RECORD             1002
#define P_ADDBUFFER          1003
#define P_BEGINPLAYBACK      1004
#define P_QUALITY            1005
#define P_USECOMPRESSION     1006
#define P_ADDBUFFER2         1007
#define P_CLEAR              1008
#define P_BANDWIDTH          1009
#define P_BITRATE            3725
#define P_MASKED             400
#define P_MAXLEN             402
#define P_STEP               802

// 2 seconds
#define MAX_LATENCY_BUF      2 * (SAMPLE_RATE * NUM_CHANNELS)
#define MAX_BUFFER           0x3FFFF

INVOKER    RemoteInvoker = 0;
AnsiString buffered_text;

#define SAMPLE               short
#define PA_SAMPLE_TYPE       paInt16
#define SAMPLE_SILENCE       (0)
#define FRAMES_PER_BUFFER    (1024)

#define EVENT_ON_BUFFER      350

static int recordCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData);

static int playCallback(const void *inputBuffer, void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void *userData);

static inline int align2(int i) {
    return (i + sizeof(void *) - 1) & - ((int)sizeof(void *));
}

int AesRoundSize(int iSize, int iAlignSize) {
    int iMul = (iSize / iAlignSize) * iAlignSize;

    if (iSize != iMul)
        iSize = iMul + iAlignSize;
    return iSize;
}

class AudioObject {
public:
    PaStreamParameters inputParameters;
    PaStreamParameters outputParameters;
    PaStream           *stream;
    PaError            err;
    int        SAMPLE_RATE;
    int        NUM_CHANNELS;
    int        finished;
    int        quality;
    int        speex_frame_size;
    bool       on_buffer_full;
    SpeexBits  bits;
    void       *enc_state;
    void       *dec_state;
    AnsiString audio_buf;
    AnsiString audio_buf2;
    SAMPLE     propagation[0xFFF];
    int        propagation_size;
    int        use_compression;
    int        bitrate;
    int        bandwidth;
    bool       quality_changed;
    HHSEM      lock;
    HHSEM      lock2;
    int        maxbuffers;
    int        framesize;
    char       DRM;
    AnsiString key;

    OpusDecoder *dec;
    OpusEncoder *enc;
    opus_int16  *out;

    AES *encrypt;
    AES *decrypt;

    AudioObject() {
        err            = paNoError;
        stream         = 0;
        SAMPLE_RATE    = 44100;
        NUM_CHANNELS   = 1;
        finished       = 0;
        on_buffer_full = false;

        enc_state        = 0;
        dec_state        = 0;
        quality          = 8;
        speex_frame_size = 0;
        seminit(lock, 1);
        seminit(lock2, 1);
        propagation_size = 0;
        use_compression  = 1;
        quality_changed  = false;
        DRM        = 0;
        bitrate    = 16000;
        maxbuffers = 0;
        dec        = 0;
        enc        = 0;
        out        = 0;
        framesize  = 20;
        encrypt    = 0;
        decrypt    = 0;
        bandwidth  = -1;
    }

    ~AudioObject() {
    }

    void InitEncryption() {
        if (encrypt) {
            delete encrypt;
            encrypt = 0;
        }
        if (DRM) {
            encrypt = new AES();
            int key_size = key.Length();
            encrypt->SetParameters(key_size * 8, key_size * 8);
            encrypt->StartEncryption((unsigned char *)key.c_str());
        }
    }

    void Encrypt(char *buf, int len) {
        if (!encrypt)
            return;

        int iRoundSize = AesRoundSize(len, 16);
        if (iRoundSize == len)
            iRoundSize += 16;

        char *filled = 0;
        char is_new  = 0;

        if (len != iRoundSize) {
            is_new = 1;
            filled = (char *)malloc(iRoundSize);
            memcpy(filled, buf, len);
            unsigned char padding = iRoundSize - len;
            memset(filled + len, padding, (int)padding);
        } else
            filled = (char *)buf;

        char *out = (char *)malloc(iRoundSize);
        encrypt->Encrypt((const unsigned char *)filled, (unsigned char *)out, iRoundSize / 16, DRM == 2 ? (AES::ECB): (AES::CBC));

        RemoteInvoker(this, INVOKE_FIREEVENT2, (int)EVENT_ON_BUFFER, out, iRoundSize);

        if (is_new)
            free(filled);

        free(out);
    }

    AnsiString Decrypt(char *buf, int len) {
        AnsiString result;

        if ((!decrypt) || (!len) || (len % 16)) {
            result.LoadBuffer(buf, len);
            return result;
        }
        char *out = (char *)malloc(len);
        if (out) {
            decrypt->Decrypt((const unsigned char *)buf, (unsigned char *)out, len / 16, DRM == 2 ? (AES::ECB): (AES::CBC));
            int padding = out[len - 1];
            int padSize = len;
            if ((padding > 0) && (padding <= 16))
                padSize -= padding;

            result.LoadBuffer(out, padSize);
            free(out);
        }

        return result;
    }

    void InitDecryption() {
        if (decrypt) {
            delete decrypt;
            decrypt = 0;
        }
        if (DRM) {
            decrypt = new AES();
            int key_size = key.Length();
            decrypt->SetParameters(key_size * 8, key_size * 8);
            decrypt->StartDecryption((unsigned char *)key.c_str());
        }
    }

    bool IsActive() {
        if (!stream)
            return false;
        if (Pa_IsStreamActive(stream) == 1)
            return true;
        return false;
    }

    bool Stop() {
        finished = 0;
        if (!stream)
            return true;

        err = Pa_CloseStream(stream);

        if (dec_state) {
            speex_decoder_destroy(dec_state);
            dec_state = 0;
        }
        if (enc_state) {
            speex_encoder_destroy(enc_state);
            enc_state = 0;
        }

        if (use_compression == 1)
            speex_bits_destroy(&bits);

        if (enc) {
            opus_encoder_destroy(enc);
            enc = 0;
        }

        if (dec) {
            opus_decoder_destroy(dec);
            dec = 0;
        }
        if (out) {
            free(out);
            out = 0;
        }

        if (encrypt) {
            delete encrypt;
            encrypt = 0;
        }

        if (decrypt) {
            delete decrypt;
            decrypt = 0;
        }

        stream = 0;
        return true;
    }

    void OpusDeBuffer(char *buf, int buf_size, int use_int_instead_of_char = 0) {
        if ((!stream) || (!dec))
            return;
        spx_int16_t internal2[0xFFFF];
        if ((!buf) || (!buf_size))
            return;

        int total     = 0;
        int remaining = 0x2FFFFF;
        if (!out)
            out = (opus_int16 *)malloc(sizeof(opus_int16) * (remaining + 1));

        opus_int16 *temp_out = out;
        if (temp_out) {
            while ((buf_size > 2) && (remaining > 0)) {
                int p_size = (unsigned char)buf[0];

                if (p_size > 0xC0) {
                    buf++;
                    buf_size--;
                    p_size -= 0xC0;
                    p_size *= 0x3F;
                    p_size += (unsigned char)buf[0];
                }
                buf++;
                buf_size--;

                if (p_size > buf_size) {
                    fprintf(stderr, "Incomplete buffer. Bytes: %i\n", p_size);
                    fflush(stderr);
                    break;
                }

                int nbsamples = opus_packet_get_nb_samples((unsigned char *)buf, p_size, SAMPLE_RATE);
                if (nbsamples > 0) {
                    int len = nbsamples;
                    if (len > remaining)
                        len = remaining;
                    int res = opus_decode(dec, (unsigned char *)buf, p_size, temp_out, len, 0);
                    if (res > 0) {
                        remaining -= res;
                        total     += res;
                        temp_out  += res;
                        if (remaining <= 0) {
                            fprintf(stderr, "Output buffer is full\n");
                            fflush(stderr);
                            break;
                        }
                    } else {
                        fprintf(stderr, "Error in decoding: %s\n", AnsiString((long)res).c_str());
                        fflush(stderr);
                        break;
                    }
                }
                buf      += p_size;
                buf_size -= p_size;
            }
        }

        semp(lock);
        int size        = audio_buf.Length();
        int packet_size = total * sizeof(short);
        if ((maxbuffers > 0) && (size / packet_size > maxbuffers))
            audio_buf.LoadBuffer((char *)out, packet_size);
        else {
            AnsiString temp;
            temp.LoadBuffer((char *)out, packet_size);
            audio_buf += temp;
        }
        semv(lock);
    }

    void SpeexDeBuffer(char *buf, int buf_size, int use_int_instead_of_char = 0) {
        if (!stream)
            return;
        spx_int16_t internal2[0xFFFF];
        if ((!buf) || (!buf_size))
            return;

        int n_frames = (unsigned char)buf[0];
        if (use_int_instead_of_char) {
            n_frames  = *(unsigned int *)&buf[0];
            buf      += sizeof(unsigned int);
            buf_size -= sizeof(unsigned int);
        } else {
            buf++;
            buf_size--;
        }

        if (!n_frames)
            return;
        int p_size = (unsigned char)buf[0]; //buf_size/n_frames;
        buf++;
        buf_size--;
        int total = 0;
        //fprintf(stderr, "Speex: init packet: %i frames of %i size\n",n_frames,p_size);
        //fflush(stderr);
        do {
            n_frames--;
            if (n_frames < 0) {
                if (use_int_instead_of_char)
                    break;
                n_frames = (unsigned char)buf[0];
                buf++;
                buf_size--;
                p_size = (unsigned char)buf[0];
                buf++;
                buf_size--;
                n_frames--;
            }
            speex_bits_reset(&bits);
            speex_bits_read_from(&bits, buf, buf_size);
            err = speex_decode_int(dec_state, &bits, (spx_int16_t *)&internal2[total]);
            if (err) {
                fprintf(stderr, "Speex: Error decoding packet: %i\n", err);
                fflush(stderr);
            }
            total += speex_frame_size;
            // dirty !
            if ((total + speex_frame_size) * sizeof(short) > sizeof(internal2)) {
                if (use_int_instead_of_char) {
                    AnsiString temp;
                    temp.LoadBuffer((char *)internal2, total * sizeof(short));
                    semp(lock);
                    audio_buf += temp;
                    semv(lock);
                    total = 0;
                } else
                    break;
            }
            buf      += p_size;
            buf_size -= p_size;
            if (buf_size < p_size)
                break;
        } while ((!err) /*&& (buf_size>=p_size)*/);
        semp(lock);
        AnsiString temp;

        int size        = audio_buf.Length();
        int packet_size = total * sizeof(short);
        if (((maxbuffers > 0) && (size / packet_size > maxbuffers)) && (!use_int_instead_of_char))
            audio_buf.LoadBuffer((char *)internal2, total * sizeof(short));
        else {
            temp.LoadBuffer((char *)internal2, total * sizeof(short));
            audio_buf += temp;
        }
        semv(lock);
    }

    int SpeexBuffer2(const SAMPLE *buf, int buf_size) {
        if (!stream)
            return -1;

        // buffer to small, will ignore it
        // that's not good
        // REWRITE THIS

        // at least 3 packages !
        if (buf_size + propagation_size < speex_frame_size * 3) {
            propagation_size = 0;
            return -2;
        }

        char internal2[0xFFFF];

        int total = 0;
        int pos   = 0;

        if (quality_changed) {
            speex_encoder_ctl(enc_state, SPEEX_SET_QUALITY, &quality);
            speex_encoder_ctl(enc_state, SPEEX_GET_FRAME_SIZE, &speex_frame_size);
            quality_changed = false;
        }

        //speex_bits_reset(&bits);
        //speex_encode_int(enc_state, (spx_int16_t *)&buf[pos], &bits);

        internal2[0] = 0; //packets !
        total++;
        //internal2[0] = 0; //p_size
        internal2[1] = 0; //p_size
        total++;
        if (propagation_size) {
            int delta = speex_frame_size - propagation_size;
            for (int i = 0; i < delta; i++)
                propagation[propagation_size + i] = buf[i];
            pos       = delta;
            buf_size -= delta;
            speex_bits_reset(&bits);
            speex_encode_int(enc_state, (spx_int16_t *)propagation, &bits);
            int b = speex_bits_write(&bits, &internal2[total], 0xFFFF - total);
            if (b > 0) {
                total           += b;
                internal2[1]     = b;
                propagation_size = 0;
                internal2[0]++;
            }
        }

        while (buf_size >= speex_frame_size) {
            speex_bits_reset(&bits);
            speex_encode_int(enc_state, (spx_int16_t *)&buf[pos], &bits);
            int nbBytes = speex_bits_write(&bits, &internal2[total], 0xFFFF - total);
            if (nbBytes <= 0)
                break;
            internal2[1] = nbBytes;
            pos         += speex_frame_size;
            buf_size    -= speex_frame_size;
            total       += nbBytes;
            internal2[0]++;
        }
        if (buf_size) {
            for (int i = 0; i < buf_size; i++)
                propagation[i] = buf[pos + i];
            propagation_size = buf_size;
        }
        if (DRM)
            Encrypt(internal2, total);
        else
            RemoteInvoker(this, INVOKE_FIREEVENT2, (int)EVENT_ON_BUFFER, internal2, total);
        return buf_size;
    }

    int OpusBuffer2(const SAMPLE *buf, int buf_size) {
        if ((!stream) || (!enc))
            return -1;
        // ignore frame_size, default it to 20ms
        int frame_size = 20 * SAMPLE_RATE / 1000;
        // at least 3 packages !
        if (buf_size + propagation_size < frame_size) {
            propagation_size = 0;
            return -2;
        }

        int   total  = 0;
        int   pos    = 0;
        short *buf_i = 0;

        if (quality_changed) {
            opus_encoder_ctl(enc, OPUS_SET_BITRATE_REQUEST, bitrate);
            opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY_REQUEST, quality);
            if (bandwidth > 0) {
                switch (bandwidth) {
                    case 1:
                        opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH_REQUEST, OPUS_BANDWIDTH_NARROWBAND);
                        opus_encoder_ctl(enc, OPUS_SET_SIGNAL_REQUEST, OPUS_SIGNAL_VOICE);
                        break;

                    case 2:
                        opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH_REQUEST, OPUS_BANDWIDTH_MEDIUMBAND);
                        opus_encoder_ctl(enc, OPUS_SET_SIGNAL_REQUEST, OPUS_SIGNAL_VOICE);
                        break;

                    case 3:
                        opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH_REQUEST, OPUS_BANDWIDTH_WIDEBAND);
                        opus_encoder_ctl(enc, OPUS_SET_SIGNAL_REQUEST, OPUS_AUTO);
                        break;

                    case 4:
                        opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH_REQUEST, OPUS_BANDWIDTH_SUPERWIDEBAND);
                        opus_encoder_ctl(enc, OPUS_SET_SIGNAL_REQUEST, OPUS_AUTO);
                        break;

                    case 5:
                        opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH_REQUEST, OPUS_BANDWIDTH_FULLBAND);
                        opus_encoder_ctl(enc, OPUS_SET_SIGNAL_REQUEST, OPUS_AUTO);
                        break;
                }
            }
            quality_changed = false;
        }

        if (propagation_size > 0) {
            buf_i = (short *)malloc((propagation_size + buf_size) * sizeof(short));
            int offset = propagation_size;
            for (int i = 0; i < offset; i++)
                buf_i[i] = propagation[i];

            for (int i = 0; i < buf_size; i++)
                buf_i[offset + i] = buf[i];
            buf_size        += offset;
            propagation_size = 0;
            buf = buf_i;
        }
        int           offset   = 0;
        int           tmp_size = 0x3FFFF;
        unsigned char out[0x3FFFF];
        unsigned char *ptr            = out;
        int           temp_frame_size = frame_size;
        while (buf_size >= frame_size) {
            /*if (buf_size >= frame_size * 3)
                    temp_frame_size=frame_size * 3;
               else*/
            if (buf_size >= frame_size * 2)
                temp_frame_size = frame_size * 2;
            else
                temp_frame_size = frame_size;
            int res = opus_encode(enc, (short int *)buf, temp_frame_size, ptr + 1, tmp_size - 2);
            if (res > 0) {
                int chunk_size;
                if (res <= 0xC0) {
                    ptr[0]     = (unsigned char)res;
                    chunk_size = 1;
                } else {
                    if (tmp_size <= res + 1) {
                        buf_size = 0;
                        fprintf(stderr, "Buffer full. Bytes: %i\n", tmp_size);
                        break;
                    }

                    for (int i = res; i >= 0; i--)
                        ptr[i] = ptr[i - 1];

                    ptr[0]     = (unsigned char)(0xC0 + res / 0x3F);
                    ptr[1]     = (unsigned char)res % 0x3F;
                    chunk_size = 2;
                }
                res      += chunk_size;
                tmp_size -= res;
                ptr      += res;
                total    += res;
            } else {
                buf_size = 0;
                fprintf(stderr, "Error encoding. Bytes: %i\n", temp_frame_size);
                break;
            }
            buf_size -= temp_frame_size;
            buf      += temp_frame_size;
        }
        if (buf_size > 0) {
            propagation_size = buf_size;
            for (int i = 0; i < buf_size; i++)
                propagation[i] = buf[i];
        }
        if (buf_i)
            free(buf_i);

        if (DRM)
            Encrypt((char *)out, total);
        else
            RemoteInvoker(this, INVOKE_FIREEVENT2, (int)EVENT_ON_BUFFER, out, total);
        return buf_size;
    }

    bool Record(int device = -1) {
        if ((stream) && (IsActive()))
            Stop();
        if ((NUM_CHANNELS == 1) && (use_compression == 1)) {
            quality_changed = false;
            speex_bits_init(&bits);
            enc_state = speex_encoder_init(&speex_nb_mode);
            speex_encoder_ctl(enc_state, SPEEX_SET_QUALITY, &quality);
            speex_encoder_ctl(enc_state, SPEEX_SET_SAMPLING_RATE, &SAMPLE_RATE);
            speex_encoder_ctl(enc_state, SPEEX_GET_FRAME_SIZE, &speex_frame_size);
        } else
        if (use_compression == 2) {
            int err = 0;
            enc = opus_encoder_create(SAMPLE_RATE, NUM_CHANNELS, OPUS_APPLICATION_VOIP, &err);
            if (err) {
                fprintf(stderr, "Error creating OPUS encoder: %i (Sample rate: %i, Channels: %i)\n", err, SAMPLE_RATE, NUM_CHANNELS);
                enc = 0;
            }

            if (enc) {
                if (bandwidth > 0) {
                    switch (bandwidth) {
                        case 1:
                            opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH_REQUEST, OPUS_BANDWIDTH_NARROWBAND);
                            opus_encoder_ctl(enc, OPUS_SET_SIGNAL_REQUEST, OPUS_SIGNAL_VOICE);
                            break;

                        case 2:
                            opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH_REQUEST, OPUS_BANDWIDTH_MEDIUMBAND);
                            opus_encoder_ctl(enc, OPUS_SET_SIGNAL_REQUEST, OPUS_SIGNAL_VOICE);
                            break;

                        case 3:
                            opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH_REQUEST, OPUS_BANDWIDTH_WIDEBAND);
                            opus_encoder_ctl(enc, OPUS_SET_SIGNAL_REQUEST, OPUS_AUTO);
                            break;

                        case 4:
                            opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH_REQUEST, OPUS_BANDWIDTH_SUPERWIDEBAND);
                            opus_encoder_ctl(enc, OPUS_SET_SIGNAL_REQUEST, OPUS_AUTO);
                            break;

                        case 5:
                            opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH_REQUEST, OPUS_BANDWIDTH_FULLBAND);
                            opus_encoder_ctl(enc, OPUS_SET_SIGNAL_REQUEST, OPUS_AUTO);
                            break;
                    }
                } else {
                    if (bitrate <= 16000) {
                        opus_encoder_ctl(enc, OPUS_SET_SIGNAL_REQUEST, OPUS_SIGNAL_VOICE);
                        if (bitrate <= 8000)
                            opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH_REQUEST, OPUS_BANDWIDTH_NARROWBAND);
                    } else {
                        opus_encoder_ctl(enc, OPUS_SET_SIGNAL_REQUEST, OPUS_AUTO);
                        if (bitrate > 8000)
                            opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH_REQUEST, OPUS_AUTO);
                    }
                }
                opus_encoder_ctl(enc, OPUS_SET_BITRATE_REQUEST, bitrate);
                opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY_REQUEST, quality);
            }
        }
        inputParameters.device = device < 0 ? Pa_GetDefaultInputDevice() : device;
        if (inputParameters.device == paNoDevice)
            return false;
        inputParameters.channelCount              = NUM_CHANNELS;
        inputParameters.sampleFormat              = PA_SAMPLE_TYPE;
        inputParameters.suggestedLatency          = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
        inputParameters.hostApiSpecificStreamInfo = NULL;

        err = Pa_OpenStream(
            &stream,
            &inputParameters,
            NULL,                        /* &outputParameters, */
            SAMPLE_RATE,
            FRAMES_PER_BUFFER,
            paClipOff,            /* we won't output out of range samples so don't bother clipping them */
            recordCallback,
            this);

        err = Pa_StartStream(stream);
        if (err != paNoError) {
            fprintf(stderr, "Error in record: %i\n", (int)err);
            return false;
        }
        return true;
    }

    bool Play(int device = -1) {
        if ((stream) && (IsActive()))
            Stop();

        if ((NUM_CHANNELS == 1) && (use_compression == 1)) {
            speex_bits_init(&bits);
            dec_state = speex_decoder_init(&speex_nb_mode);
            speex_decoder_ctl(dec_state, SPEEX_SET_SAMPLING_RATE, &SAMPLE_RATE);
            speex_decoder_ctl(dec_state, SPEEX_GET_FRAME_SIZE, &speex_frame_size);
        } else
        if (use_compression == 2) {
            //dec=(OpusDecoder *)malloc(opus_decoder_get_size(NUM_CHANNELS)+align2(2));
            //int err = opus_decoder_init(dec, SAMPLE_RATE, NUM_CHANNELS);
            int err = 0;
            dec = opus_decoder_create(SAMPLE_RATE, NUM_CHANNELS, &err);
            if (err) {
                fprintf(stderr, "Error creating OPUS decoder: %i (Sample rate: %i, Channels: %i)\n", err, SAMPLE_RATE, NUM_CHANNELS);
                free(dec);
                dec = NULL;
            }
        }

        outputParameters.device = device < 0 ? Pa_GetDefaultOutputDevice() : device;
        if (inputParameters.device == paNoDevice)
            return false;
        outputParameters.channelCount              = NUM_CHANNELS;
        outputParameters.sampleFormat              = PA_SAMPLE_TYPE;
        outputParameters.suggestedLatency          = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = NULL;

        err = Pa_OpenStream(
            &stream,
            NULL,
            &outputParameters,
            SAMPLE_RATE,
            /*FRAMES_PER_BUFFER*/ SAMPLE_RATE / 50,
            paClipOff,
            playCallback,
            this);

        err = Pa_StartStream(stream);
        if (err != paNoError)
            return false;
        return true;
    }

    void ClearBuffer() {
        semp(lock);
        audio_buf = "";
        semv(lock);
    }

    void AddBuffer(void *str_buffer, int buf_len, int _use_int = 0) {
        if (!stream)
            return;

        AnsiString res;
        if (DRM) {
            res        = Decrypt((char *)str_buffer, buf_len);
            buf_len    = res.Length();
            str_buffer = res.c_str();
        }
        if ((NUM_CHANNELS == 1) && (use_compression == 1))
            SpeexDeBuffer((char *)str_buffer, buf_len, _use_int);
        else
        if (use_compression == 2) {
            OpusDeBuffer((char *)str_buffer, buf_len, _use_int);
        } else {
            semp(lock);
            int size        = audio_buf.Length();
            int packet_size = buf_len * sizeof(short);
            //if (audio_buf.Length() > MAX_LATENCY_BUF)
            if ((maxbuffers > 0) && (size / packet_size > maxbuffers))
                audio_buf.LoadBuffer((char *)str_buffer, buf_len);
            else {
                AnsiString temp;
                temp.LoadBuffer((char *)str_buffer, buf_len);
                audio_buf += temp;
            }
            semv(lock);
        }
    }
};

static int playCallback(const void *inputBuffer, void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void *userData) {
    AudioObject   *data = (AudioObject *)userData;
    SAMPLE        *wptr = (SAMPLE *)outputBuffer;
    unsigned long size  = framesPerBuffer * data->NUM_CHANNELS * sizeof(SAMPLE);

    int finished = data->finished;
    int rsize    = 0;
    int rd       = 0;
    int left     = size;

    //while (left) {
    if (left) {
        semp(data->lock);
        int len = data->audio_buf.Length();
        if (len >= left) {
            memcpy(wptr, data->audio_buf.c_str(), left);
            if (len == left)
                data->audio_buf = (char *)"";
            else {
                AnsiString temp;
                temp.LoadBuffer(data->audio_buf.c_str() + left, len - left);
                data->audio_buf = temp;
            }
            left = 0;
        }
        semv(data->lock);
    }
    return finished;
}

static int recordCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData) {
    AudioObject  *data    = (AudioObject *)userData;
    const SAMPLE *rptr    = (const SAMPLE *)inputBuffer;
    int          size2    = framesPerBuffer * data->NUM_CHANNELS;
    int          size     = size2 * sizeof(SAMPLE);
    int          finished = data->finished;

    if (data->on_buffer_full) {
        if ((data->NUM_CHANNELS == 1) && (data->use_compression == 1))
            data->SpeexBuffer2(rptr, size2);
        else
        if (data->use_compression == 2)
            data->OpusBuffer2(rptr, size2);
        else {
            if (data->DRM)
                data->Encrypt((char *)rptr, size);
            else {
                RemoteInvoker(data, INVOKE_FIREEVENT2, (int)EVENT_ON_BUFFER, rptr, size);
            }
        }
    }
    return finished;
}

PREFIX int Invoke(void *context, int INVOKE_TYPE, ...) {
    va_list arg;

    switch (INVOKE_TYPE) {
        case INVOKE_QUERYTEST:
            {
                va_start(arg, INVOKE_TYPE);
                int cls_id = va_arg(arg, int);
                va_end(arg);

                if (cls_id == CUSTOM_CONTROL_ID)
                    return E_ACCEPTED;
            }
            break;

        case INVOKE_SETEVENT:
            {
                if (!RemoteInvoker)
                    break;
                va_start(arg, INVOKE_TYPE);
                int  cls_id      = va_arg(arg, int);
                int  event_id    = va_arg(arg, int);
                char *str_buffer = va_arg(arg, char *);
                int  buf_len     = va_arg(arg, int);
                va_end(arg);

                if ((!context) || (cls_id != CUSTOM_CONTROL_ID))
                    break;

                AudioObject *ctx = (AudioObject *)context;
                switch (event_id) {
                    case EVENT_ON_BUFFER:
                        ctx->on_buffer_full = true;
                        return E_ACCEPTED;
                        break;
                }
            }
            break;

        case INVOKE_SETPROPERTY:
            {
                va_start(arg, INVOKE_TYPE);
                int  cls_id      = va_arg(arg, int);
                int  prop_id     = va_arg(arg, int);
                char *str_buffer = va_arg(arg, char *);
                int  buf_len     = va_arg(arg, int);
                va_end(arg);
                AnsiString param(str_buffer);
                int        param_int = param.ToInt();

                if ((!context) || (cls_id != CUSTOM_CONTROL_ID))
                    break;

                AudioObject *ctx = (AudioObject *)context;
                switch (prop_id) {
                    case P_SAMPLERATE:
                        ctx->SAMPLE_RATE = param.ToInt();
                        break;

                    case P_NUMCHANNELS:
                        ctx->NUM_CHANNELS = param.ToInt();
                        break;

                    case P_RECORD:
                        if (param_int == -2)
                            ctx->Stop();
                        else {
                            if (ctx->DRM) {
                                char *key = NULL;
                                int  len  = 0;
                                RemoteInvoker(context, INVOKE_GETPRIVATEKEY, &key, &len);
                                if ((key) && (len)) {
                                    ctx->key.LoadBuffer(key, len);
                                    ctx->InitEncryption();
                                } else
                                    ctx->DRM = 0;
                            }
                            ctx->Record(param_int);
                        }
                        break;

                    case P_ADDBUFFER:
                        ctx->AddBuffer(str_buffer, buf_len);
                        break;

                    case P_ADDBUFFER2:
                        ctx->AddBuffer(str_buffer, buf_len, 1);
                        break;

                    case P_BEGINPLAYBACK:
                        if (param_int == -2)
                            ctx->Stop();
                        else {
                            if (ctx->DRM) {
                                char *key = NULL;
                                int  len  = 0;
                                RemoteInvoker(context, INVOKE_GETPRIVATEKEY, &key, &len);
                                if ((key) && (len)) {
                                    ctx->key.LoadBuffer(key, len);
                                    ctx->InitDecryption();
                                } else
                                    ctx->DRM = 0;
                            }
                            ctx->Play(param_int);
                        }
                        break;

                    case P_QUALITY:
                        ctx->quality         = param.ToInt();
                        ctx->quality_changed = true;
                        break;

                    case P_USECOMPRESSION:
                        ctx->use_compression = param.ToInt();
                        break;

                    case P_CLEAR:
                        break;

                    case P_MASKED:
                        ctx->DRM = param.ToInt();
                        break;

                    case P_BITRATE:
                        ctx->bitrate         = param.ToInt();
                        ctx->quality_changed = true;
                        break;

                    case P_BANDWIDTH:
                        ctx->bandwidth       = param.ToInt();
                        ctx->quality_changed = true;
                        break;

                    case P_MAXLEN:
                        ctx->maxbuffers = param.ToInt();
                        break;

                    case P_STEP:
                        ctx->framesize = param.ToInt();
                        break;
                }
            }
            break;

        case INVOKE_GETPROPERTY:
            {
                va_start(arg, INVOKE_TYPE);
                int  cls_id       = va_arg(arg, int);
                int  prop_id      = va_arg(arg, int);
                char **str_buffer = va_arg(arg, char **);
                int  *buf_len     = va_arg(arg, int *);
                va_end(arg);

                if ((!context) || (cls_id != CUSTOM_CONTROL_ID))
                    break;

                buffered_text = (char *)"";
                *str_buffer   = buffered_text.c_str();
                *buf_len      = buffered_text.Length();
                return E_NOERROR;
            }
            break;

        case INVOKE_SETSTREAM:
            {
                va_start(arg, INVOKE_TYPE);
                int  cls_id     = va_arg(arg, int);
                char *target    = va_arg(arg, char *);
                int  len_target = va_arg(arg, int);
                char *value     = va_arg(arg, char *);
                int  len_value  = va_arg(arg, int);
                va_end(arg);

                if ((!context) || (cls_id != CUSTOM_CONTROL_ID))
                    break;
            }
            break;

        case INVOKE_SETSECONDARYSTREAM:
            // not needed
            break;

        case INVOKE_CUSTOMMESSAGE:
            {
            }
            break;

        case INVOKE_CREATE:
            {
                va_start(arg, INVOKE_TYPE);
                int  cls_id     = va_arg(arg, int);
                void **handler  = va_arg(arg, void **);
                int  *pack_type = va_arg(arg, int *);
                va_end(arg);
                *pack_type = (int)2;

                if (cls_id != CUSTOM_CONTROL_ID)
                    break;

                AudioObject *ctx = new AudioObject();
                *handler = ctx;
                return E_ACCEPTED_NOADD;
            }
            break;

        case INVOKE_DESTROY:
            {
                va_start(arg, INVOKE_TYPE);
                int  cls_id   = va_arg(arg, int);
                void *handler = va_arg(arg, void *);
                va_end(arg);

                if (cls_id != CUSTOM_CONTROL_ID)
                    break;

                return E_ACCEPTED;
            }
            break;

        case INVOKE_INITLIBRARY:
            va_start(arg, INVOKE_TYPE);
            RemoteInvoker = va_arg(arg, INVOKER);
            va_end(arg);
            Pa_Initialize();
            return E_NOERROR;

        case INVOKE_DONELIBRARY:
            return E_NOERROR;

        default:
            return E_NOTIMPLEMENTED;
    }
    return E_NOTIMPLEMENTED;
}
