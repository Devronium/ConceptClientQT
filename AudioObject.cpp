#include "AudioObject.h"

int AesRoundSize2(int iSize, int iAlignSize) {
    int iMul = (iSize/iAlignSize) * iAlignSize;
    if(iSize != iMul)
        iSize = iMul + iAlignSize;
    return  iSize;
}


AudioInfo::AudioInfo(AudioObject *owner, const QAudioFormat &format, QObject *parent) : QIODevice(parent) {
    this->owner = owner;
}

AudioInfo::~AudioInfo() {
    
}

void AudioInfo::start(bool recording) {
    if (recording)
        open(QIODevice::WriteOnly);
    else
        open(QIODevice::ReadOnly);
}

void AudioInfo::stop() {
    close();
}

qint64 AudioInfo::readData(char *data, qint64 maxlen) {
    int len = 0;
    if (maxlen <= 0)
        return 0;
    
    qint64 origlen = maxlen;
    semp(owner->lock);
    int temp_len = owner->audio_buf.Length();
    if (temp_len < maxlen)
        maxlen = temp_len;
    if (maxlen) {
        len = maxlen;
        memcpy(data, owner->audio_buf.c_str(), maxlen);
        AnsiString temp;
        if (temp_len > maxlen)
            temp.LoadBuffer(owner->audio_buf.c_str() + maxlen, temp_len - maxlen);

        owner->audio_buf = temp;
    }
    semv(owner->lock);
    if ((data) && (!len) && (origlen > 0)) {
        int buf_size = 160;
        if ((owner->framesize > 0) && (owner->SAMPLE_RATE > 0))
            buf_size = owner->SAMPLE_RATE * owner->framesize / 1000;
    
        if (owner->NUM_CHANNELS > 1)
            buf_size *= owner->NUM_CHANNELS;

        buf_size *= 2;
        if (origlen < buf_size)
            buf_size = origlen;

        fprintf(stderr, "Buffer underrun\n");
        memset(data, 0, buf_size);
        len = buf_size;
    }
    return len;
}

qint64 AudioInfo::writeData(const char *data, qint64 len) {
    if (this->owner->on_buffer_full) {
#ifndef NO_SPEEX
        if ((this->owner->NUM_CHANNELS == 1) && (this->owner->use_compression == 1))
            this->owner->SpeexBuffer2((SAMPLE *)data, len / 2);
        else
#endif
#ifndef NO_OPUS
        if (this->owner->use_compression == 2)
            this->owner->OpusBuffer2((short *)data, len / 2);
        else 
#endif
        {
            if (this->owner->DRM)
                this->owner->Encrypt((char *)data, len);
            else {
                AnsiString buf;
                buf.LoadBuffer((char *)data, len);
                this->owner->owner->SendMessageNoWait(this->owner->ID.c_str(), MSG_EVENT_FIRED, "350", buf);
            }
        }
    }
    return len;
}


AudioObject::AudioObject(CConceptClient *owner, long ID) {
    this->ID       = AnsiString(ID);
    this->owner    = owner;
    stream_type    = 0;
    err            = 0;
    stream         = 0;
    audioInfo      = 0;
    SAMPLE_RATE    = 44100;
    NUM_CHANNELS   = 1;
    finished       = 0;
    on_buffer_full = false;
#ifndef NO_SPEEX
    enc_state        = 0;
    dec_state        = 0;
#endif
    quality          = 8;
    speex_frame_size = 0;
    seminit(lock, 1);
    propagation_size = 0;
    use_compression  = 1;
    quality_changed  = false;
    DRM              = 0;
    bitrate          = 16000;
    maxbuffers       = 0;
#ifndef NO_OPUS
    dec              = 0;
    enc              = 0;
    out              = 0;
#endif
    framesize        = 20;
    encrypt          = 0;
    decrypt          = 0;
    bandwidth        = -1;
}

AudioObject::~AudioObject() {
}

void AudioObject::InitEncryption() {
    if (encrypt) {
        delete encrypt;
        encrypt = 0;
    }
    if (DRM) {
        encrypt=new AES();
        int key_size=key.Length();
        encrypt->SetParameters(key_size*8,key_size*8);
        encrypt->StartEncryption((unsigned char *)key.c_str());
    }
}

void AudioObject::Encrypt(char *buf, int len) {
    if (!encrypt)
        return;
    
    int iRoundSize = AesRoundSize2(len, 16);
    if (iRoundSize==len)
        iRoundSize+=16;
    
    char *filled=0;
    char is_new=0;
    
    if (len!=iRoundSize) {
        is_new=1;
        filled=(char *)malloc(iRoundSize);
        memcpy(filled, buf, len);
        unsigned char padding=iRoundSize-len;
        memset(filled+len, padding, (int)padding);
    } else
        filled=(char *)buf;
    
    char *out=(char *)malloc(iRoundSize);
    encrypt->Encrypt((const unsigned char *)filled, (unsigned char *)out, iRoundSize/16, DRM==2 ? (AES::ECB) : (AES::CBC));
    
    AnsiString temp_buf;
    temp_buf.LoadBuffer(out, iRoundSize);

    this->owner->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, "350", temp_buf);
    
    if (is_new)
        free(filled);
    
    free(out);
}

void AudioObject::Decrypt(AnsiString *result, char *buf, int len) {
    if ((!decrypt) || (!len) || (len%16)) {
        result->LoadBuffer(buf, len);
        return;
    }
    char *out=(char *)malloc(len);
    if (out) {
        decrypt->Decrypt((const unsigned char *)buf, (unsigned char *)out, len/16, DRM==2 ? (AES::ECB) : (AES::CBC));
        int padding=out[len-1];
        int padSize=len;
        if ((padding>0) && (padding<=16))
            padSize-=padding;
        result->LoadBuffer(out, padSize);
        free(out);
    }
}

void AudioObject::InitDecryption() {
    if (decrypt) {
        delete decrypt;
        decrypt = 0;
    }
    if (DRM) {
        decrypt=new AES();
        int key_size=key.Length();
        decrypt->SetParameters(key_size*8,key_size*8);
        decrypt->StartDecryption((unsigned char *)key.c_str());
    }
}

bool AudioObject::IsActive() {
    if (!stream)
        return(false);

    return(true);
}

bool AudioObject::Stop() {
    finished = 0;
    if (!stream)
        return(true);
    //err = Pa_CloseStream(stream);
    
#ifndef NO_SPEEX
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
#endif
#ifndef NO_OPUS    
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
#endif
    
    if (encrypt) {
        delete encrypt;
        encrypt = 0;
    }
    
    if (decrypt) {
        delete decrypt;
        decrypt = 0;
    }
    
    if (stream_type == 1)
        ((QAudioInput *)stream)->deleteLater();
    else
    if (stream_type == 2)
        ((QAudioOutput *)stream)->deleteLater();
    stream = 0;
    
    // deleted by parent
    if (audioInfo)
        audioInfo = 0;

    return(true);
}

#ifndef NO_OPUS
void AudioObject::OpusDeBuffer(char *buf, int buf_size, int use_int_instead_of_char) {
    if ((!stream) || (!dec))
        return;
    spx_int16_t internal2[0xFFFF];
    if ((!buf) || (!buf_size))
        return;
    
    int total = 0;
    int remaining=0x2FFFFF;
    if (!out)
        out=(opus_int16 *)malloc(sizeof(opus_int16)*(remaining+1));
    
    opus_int16 *temp_out=out;
    if (temp_out) {
        while ((buf_size>2) && (remaining>0)) {
            int p_size = (unsigned char)buf[0];
            
            if (p_size>0xC0) {
                buf++;
                buf_size--;
                p_size-=0xC0;
                p_size*=0x3F;
                p_size+=(unsigned char)buf[0];
            }
            buf++;
            buf_size--;
            
            if (p_size>buf_size) {
                fprintf(stderr, "Incomplete buffer. Bytes: %i\n", p_size);
                fflush(stderr);
                break;
            }
            
            int nbsamples=opus_packet_get_nb_samples((unsigned char *)buf, p_size, SAMPLE_RATE);
            if (nbsamples>0) {
                int len = nbsamples;
                if (len > remaining)
                    len = remaining;
                int res=opus_decode(dec, (unsigned char *)buf, p_size, temp_out, len, 0);
                if (res>0) {
                    remaining-=res;
                    total+=res;
                    temp_out+=res;
                    if (remaining<=0) {
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
            buf+=p_size;
            buf_size-=p_size;
        }
    }
    
    semp(lock);
    int size = audio_buf.Length();
    int packet_size = total * sizeof(short);
    if ((maxbuffers>0) && (size/packet_size>maxbuffers)) {
        fprintf(stderr, "Jitter (Opus)\n");
        audio_buf.LoadBuffer((char *)out, packet_size);
    } else {
        AnsiString temp;
        temp.LoadBuffer((char *)out, packet_size);
        audio_buf += temp;
    }
    semv(lock);
}
#endif

#ifndef NO_SPEEX
void AudioObject::SpeexDeBuffer(char *buf, int buf_size, int use_int_instead_of_char) {
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
    
    int size = audio_buf.Length();
    int packet_size = total * sizeof(short);
    if (((maxbuffers>0) && (size/packet_size>maxbuffers)) && (!use_int_instead_of_char)) {
        fprintf(stderr, "Jitter (Speex)\n");
        audio_buf.LoadBuffer((char *)internal2, total * sizeof(short));
    } else {
        temp.LoadBuffer((char *)internal2, total * sizeof(short));
        audio_buf += temp;
    }
    semv(lock);
}

int AudioObject::SpeexBuffer2(const SAMPLE *buf, int buf_size) {
    if (!stream)
        return(-1);
  
    // buffer to small, will ignore it
    // that's not good
    // REWRITE THIS
    
    // at least 3 packages !
    if (buf_size + propagation_size < speex_frame_size) {
        propagation_size = 0;
        return(-2);
    }
    
    char internal2[0xFFFF];
    
    int total = 0;
    int pos   = 0;
    
    if (quality_changed) {
        speex_encoder_ctl(enc_state, SPEEX_SET_QUALITY, &quality);
        speex_encoder_ctl(enc_state, SPEEX_GET_FRAME_SIZE, &speex_frame_size);
        quality_changed=false;
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
    if (total <= 0)
        return buf_size;
    if (DRM)
        Encrypt(internal2, total);
    else {
        AnsiString buf;
        buf.LoadBuffer((char *)internal2, total);
        this->owner->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, "350", buf);
    }
    return(buf_size);
}
#endif

#ifndef NO_OPUS
int AudioObject::OpusBuffer2(const SAMPLE *buf, int buf_size) {
    if ((!stream) || (!enc))
        return(-1);
    // ignore frame_size, default it to 20ms
    int frame_size = 20 * SAMPLE_RATE / 1000 ;
    // at least 3 packages !
    //if (buf_size + propagation_size < frame_size) {
    //    propagation_size = 0;
    //    return(-2);
    //}
    
    int total = 0;
    int pos   = 0;
    short *buf_i=0;
    
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
        quality_changed=false;
    }
    
    if (propagation_size>0) {
        buf_i=(short *)malloc((propagation_size+buf_size)*sizeof(short));
        int offset=propagation_size;
        for (int i=0;i<offset;i++)
            buf_i[i]=propagation[i];
        
        for (int i=0;i<buf_size;i++)
            buf_i[offset+i]=buf[i];
        buf_size+=offset;
        propagation_size=0;
        buf=buf_i;
    }
    int offset=0;
    int tmp_size=0x3FFFF;
    unsigned char out[0x3FFFF];
    unsigned char *ptr=out;
    int temp_frame_size=frame_size;
    while (buf_size >= frame_size) {
        /*if (buf_size >= frame_size * 3)
         temp_frame_size=frame_size * 3;
         else*/
        if (buf_size >= frame_size * 2)
            temp_frame_size=frame_size * 2;
        else
            temp_frame_size=frame_size;
        int res=opus_encode(enc, (short int *)buf, temp_frame_size, ptr+1, tmp_size-2);
        if (res>0) {
            int chunk_size;
            if (res<=0xC0) {
                ptr[0]=(unsigned char)res;
                chunk_size = 1;
            } else {
                if (tmp_size<=res+1) {
                    buf_size=0;
                    fprintf(stderr, "Buffer full. Bytes: %i\n", tmp_size);
                    break;
                }
                
                for (int i=res;i>=0;i--)
                    ptr[i]=ptr[i-1];
                
                ptr[0]= (unsigned char)(0xC0 + res / 0x3F);
                ptr[1]=(unsigned char)res % 0x3F;
                chunk_size = 2;
            }
            res+=chunk_size;
            tmp_size-=res;
            ptr+=res;
            total+=res;
        } else {
            buf_size=0;
            fprintf(stderr, "Error encoding. Bytes: %i\n", temp_frame_size);
            break;
        }
        buf_size-=temp_frame_size;
        buf+=temp_frame_size;
    }
    if (buf_size>0) {
        propagation_size=buf_size;
        for (int i=0;i<buf_size;i++)
            propagation[i]=buf[i];
    }
    if (buf_i)
        free(buf_i);
    
    if (total <= 0)
        return buf_size;
    
    if (DRM)
        Encrypt((char *)out, total);
    else {
        AnsiString temp_buf;
        temp_buf.LoadBuffer((char *)out, total);
        this->owner->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, "350", temp_buf);
    }
    return(buf_size);
}
#endif

bool AudioObject::Record(int device) {
    if (IsActive())
        Stop();
#ifndef NO_SPEEX
    if ((NUM_CHANNELS == 1) && (use_compression==1)) {
        quality_changed=false;
        speex_bits_init(&bits);
        enc_state = speex_encoder_init(&speex_nb_mode);
        speex_encoder_ctl(enc_state, SPEEX_SET_QUALITY, &quality);
        speex_encoder_ctl(enc_state, SPEEX_SET_SAMPLING_RATE, &SAMPLE_RATE);
        speex_encoder_ctl(enc_state, SPEEX_GET_FRAME_SIZE, &speex_frame_size);
    } else 
#endif
#ifndef NO_OPUS
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
                if (bitrate<=16000) {
                    opus_encoder_ctl(enc, OPUS_SET_SIGNAL_REQUEST, OPUS_SIGNAL_VOICE);
                    if (bitrate<=8000)
                        opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH_REQUEST, OPUS_BANDWIDTH_NARROWBAND);
                } else {
                    opus_encoder_ctl(enc, OPUS_SET_SIGNAL_REQUEST, OPUS_AUTO);
                    if (bitrate>8000)
                        opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH_REQUEST, OPUS_AUTO);
                }
            }
            opus_encoder_ctl(enc, OPUS_SET_BITRATE_REQUEST, bitrate);
            opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY_REQUEST, quality);
        }
    }
#endif
    QAudioFormat format;
    format.setSampleRate(SAMPLE_RATE);
    format.setChannelCount(NUM_CHANNELS);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    QAudioInput *input_stream  = new QAudioInput(format);
    audioInfo  = new AudioInfo(this, format, input_stream);
    audioInfo->start(true);
    int buf_size = 160;
    if ((framesize > 0) && (SAMPLE_RATE > 0))
        buf_size = SAMPLE_RATE * framesize / 1000;

    if (NUM_CHANNELS > 1)
        buf_size *= NUM_CHANNELS;
    buf_size *= 2;
    input_stream->setBufferSize(buf_size);
    input_stream->start(audioInfo);
    stream = input_stream;
    stream_type = 1;
    return(true);
}

bool AudioObject::Play(int device) {
    if (IsActive())
        Stop();
#ifndef NO_SPEEX
    if ((NUM_CHANNELS == 1) && (use_compression == 1)) {
        speex_bits_init(&bits);
        dec_state = speex_decoder_init(&speex_nb_mode);
        speex_decoder_ctl(dec_state, SPEEX_SET_SAMPLING_RATE, &SAMPLE_RATE);
        speex_decoder_ctl(dec_state, SPEEX_GET_FRAME_SIZE, &speex_frame_size);
    } else
#endif
#ifndef NO_OPUS
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
#endif
    QAudioFormat format;
    format.setSampleRate(SAMPLE_RATE);
    format.setChannelCount(NUM_CHANNELS);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    
    QAudioOutput *output_stream  = new QAudioOutput(format);
    audioInfo  = new AudioInfo(this, format, output_stream);
    audioInfo->start(false);
    int buf_size = 160;
    if ((framesize > 0) && (SAMPLE_RATE > 0))
        buf_size = SAMPLE_RATE * framesize / 1000;
    
    if (NUM_CHANNELS > 1)
        buf_size *= NUM_CHANNELS;
    buf_size *= 2;
    output_stream->setBufferSize(buf_size * 8);
    output_stream->start(audioInfo);
    stream = output_stream;
    stream_type = 2;
    return(true);
}

void AudioObject::ClearBuffer() {
    semp(lock);
    audio_buf="";
    semv(lock);
}

void AudioObject::AddBuffer(void *str_buffer, int buf_len, int _use_int) {
    if (!stream)
        return;

    AnsiString res;
    if (DRM) {
        Decrypt(&res, (char *)str_buffer, buf_len);
        buf_len = res.Length();
        str_buffer = res.c_str();
    }
#ifndef NO_SPEEX
    if ((NUM_CHANNELS == 1) && (use_compression == 1))
        SpeexDeBuffer((char *)str_buffer, buf_len, _use_int);
    else
#endif
#ifndef NO_OPUS
    if (use_compression == 2) {
        OpusDeBuffer((char *)str_buffer, buf_len, _use_int);
    } else 
#endif
    {
        semp(lock);
        int size = audio_buf.Length();
        int packet_size = buf_len * sizeof(short);
        //if (audio_buf.Length() > MAX_LATENCY_BUF)
        if ((maxbuffers>0) && (size/packet_size>maxbuffers)) {
            audio_buf.LoadBuffer((char *)str_buffer, buf_len);
            fprintf(stderr, "Jitter (PCM)\n");
        } else {
            AnsiString temp;
            temp.LoadBuffer((char *)str_buffer, buf_len);
            audio_buf += temp;
        }
        semv(lock);
    }
}

