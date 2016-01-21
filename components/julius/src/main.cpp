//------------ standard header -----------------------------------//
#include "stdlibrary.h"
//------------ end of standard header ----------------------------//
#include <stdarg.h>
#ifdef _WIN32
 #include <windows.h>
 #include <fcntl.h>
 #define THREAD_TYPE    DWORD WINAPI
#else
 #include <pthread.h>
 #define THREAD_TYPE    LPVOID
 #define LPVOID         void *
#endif

#include "AnsiString.h"

#include <julius/julius.h>
#include <unistd.h>
#include <stdio.h>

#define CUSTOM_CONTROL_ID         1017

#define P_STARTRECOGNITION        1000
#define P_PAUSERECOGNITION        1001
#define P_TERMINATERECOGNITION    1002

#define P_ConfFile                1003
#define P_DFAGram                 1004
#define P_DictFile                1005
#define P_HMMDefs                 1006
#define P_HMMList                 1007
#define P_Penalty1                1008
#define P_Penalty2                1009
#define P_CCD                     1010
#define P_IWCD1                   1011
#define P_Mixtures                1012
#define P_Prunning                1013
#define P_GSHMM                   1014
#define P_GSNum                   1015
#define P_BeamWidth               1016
#define P_BeamWidth2              1017
#define P_ScoreBeam               1018
#define P_HypothesesStack         1019
#define P_HypothesesOverflow      1020
#define P_LookupRange             1021
#define P_Sentences               1022
#define P_Output                  1023
#define P_LookTrellis             1024
#define P_SPModel                 1025
#define P_iwsp                    1026
#define P_iwsppenalty             1027
#define P_GMM                     1028
#define P_GMMNum                  1029
#define P_GMMReject               1030
#define P_RejectShort             1031
#define P_PauseSegment            1032
#define P_TresholdLevel           1033
#define P_HeadMargin              1034
#define P_TailMargin              1035
#define P_ZeroCross               1036
#define P_SampleRate              1037
#define P_SamplePeriod            1038
#define P_WindowSize              1039
#define P_FrameShift              1040
#define P_DeltaWindow             1041
#define P_HiFreq                  1042
#define P_LoFreq                  1043
#define P_WAlign                  1044
#define P_PAlign                  1045
#define P_SAlign                  1046
#define P_CMAlpha                 1047
#define P_SeparateScore           1048
#define P_ProgOut                 1049
#define P_ProgInterval            1050
#define P_Quiet                   1051
#define P_Demo                    1052
#define P_Debug                   1053
#define P_Module                  1054
#define P_OutCode                 1055
#define P_GramFile                1056
#define P_GramList                1057

#define P_RUNNING                 1058
#define P_MICERROR                1059



#define MAX_PARAMS    0xFFF

INVOKER    RemoteInvoker = 0;
AnsiString buffered_text;

#define EVENT_ON_SENTENCE    350
#define EVENT_ON_RECSTART    351
#define EVENT_ON_RECREADY    352

THREAD_TYPE RecognitionThread(LPVOID SENDER);

class JuliusObject {
public:
    AnsiString ConfFile;

    AnsiString DFAGram;
    AnsiString DictFile;

    AnsiString HMMDefs;
    AnsiString HMMList;

    AnsiString Penalty1;
    AnsiString Penalty2;

    AnsiString CCD;

    AnsiString IWCD1;

    AnsiString Mixtures;
    AnsiString Prunning;

    AnsiString GSHMM;
    AnsiString GSNum;

    AnsiString BeamWidth;
    AnsiString BeamWidth2;
    AnsiString ScoreBeam;
    AnsiString HypothesesStack;
    AnsiString HypothesesOverflow;
    AnsiString LookupRange;
    AnsiString Sentences;
    AnsiString Output;
    AnsiString LookTrellis;
    AnsiString SPModel;

    AnsiString iwsp;
    AnsiString iwsppenalty;

    AnsiString GMM;
    AnsiString GMMNum;
    AnsiString GMMReject;

    AnsiString RejectShort;
    AnsiString PauseSegment;

    AnsiString TresholdLevel;
    AnsiString HeadMargin;
    AnsiString TailMargin;
    AnsiString ZeroCross;

    AnsiString SampleRate;
    AnsiString SamplePeriod;
    AnsiString WindowSize;
    AnsiString FrameShift;
    AnsiString DeltaWindow;
    AnsiString HiFreq;
    AnsiString LoFreq;

    AnsiString WAlign;
    AnsiString PAlign;
    AnsiString SAlign;

    AnsiString CMAlpha;

    AnsiString SeparateScore;
    AnsiString ProgOut;
    AnsiString ProgInterval;
    AnsiString Quiet;
    AnsiString Demo;

    AnsiString Debug;

    AnsiString Module;
    AnsiString OutCode;

    AnsiString GramFile;
    AnsiString GramList;

    Recog *recog;
    bool  MicError;
    bool  on_sentece;
    bool  on_recstart;
    bool  on_recready;

    JuliusObject() {
        recog       = 0;
        MicError    = false;
        on_sentece  = false;
        on_recstart = false;
        on_recready = false;
    }

    void Run() {
        if (recog)
            fprintf(stderr, "Julian: already running !\n");

#ifdef _WIN32
        DWORD TID;
        CreateThread(0, 0, RecognitionThread, this, 0, &TID);
#else
        pthread_t threadID;
        pthread_create(&threadID, NULL, RecognitionThread, this);
#endif
    }

    void Terminate() {
        if (recog)
            j_request_terminate(recog);
    }

    void Pause(bool flag = true) {
        if (recog) {
            if (flag)
                j_request_pause(recog);
            else
                j_request_resume(recog);
        }
    }

    ~JuliusObject() {
    }
};

static void status_recready(Recog *recog, void *SENDER) {
    JuliusObject *jo = (JuliusObject *)SENDER;

    if ((jo->recog->jconf->input.speech_input == SP_MIC) || (jo->recog->jconf->input.speech_input == SP_NETAUDIO))
        if (jo->on_recready)
            RemoteInvoker(jo, INVOKE_FIREEVENT, (int)EVENT_ON_RECREADY, (char *)"");
    //EVENT !//fprintf(stderr, "<<< please speak >>>");
}

static void status_recstart(Recog *recog, void *SENDER) {
    JuliusObject *jo = (JuliusObject *)SENDER;

    if ((jo->recog->jconf->input.speech_input == SP_MIC) || (jo->recog->jconf->input.speech_input == SP_NETAUDIO))
        if (jo->on_recstart)
            RemoteInvoker(jo, INVOKE_FIREEVENT, (int)EVENT_ON_RECSTART, (char *)"");
}

static AnsiString put_hypo_phoneme(WORD_ID *seq, int n, WORD_INFO *winfo) {
    int         i, j;
    WORD_ID     w;
    AnsiString  res;
    static char buf[MAX_HMMNAME_LEN];

    if (seq != NULL)
        for (i = 0; i < n; i++) {
            if (i > 0)
                res += (char *)" |";
            w = seq[i];
            for (j = 0; j < winfo->wlen[w]; j++) {
                if (j)
                    res += (char *)" ";
                center_name(winfo->wseq[w][j]->name, buf);
                res += buf;
            }
        }
    return res;
}

static void output_result(Recog *recog, void *SENDER) {
    int          i, j;
    int          len;
    WORD_INFO    *winfo;
    WORD_ID      *seq;
    int          seqnum;
    int          n;
    Sentence     *s;
    RecogProcess *r;
    HMM_Logical  *p;
    //SentenceAlign *align;
    JuliusObject *jo = (JuliusObject *)SENDER;

    /* all recognition results are stored at each recognition process
     * instance */
    AnsiString Sentences("<sl>\n");
    bool       has_data = false;

    for (r = recog->process_list; r; r = r->next) {
        /* skip the process if the process is not alive */
        if (!r->live)
            continue;

        /* result are in r->result.  See recog.h for details */

        /* check result status */
        if (r->result.status < 0) {         /* no results obtained */
            /* outout message according to the status code */
            switch (r->result.status) {
                case J_RESULT_STATUS_REJECT_POWER:
                    fprintf(stderr, "Julian: <input rejected by power>\n");
                    break;

                case J_RESULT_STATUS_TERMINATE:
                    fprintf(stderr, "Julian: <input teminated by request>\n");
                    break;

                case J_RESULT_STATUS_ONLY_SILENCE:
                    fprintf(stderr, "Julian: <input rejected by decoder (silence input result)>\n");
                    break;

                case J_RESULT_STATUS_REJECT_GMM:
                    fprintf(stderr, "Julian: <input rejected by GMM>\n");
                    break;

                case J_RESULT_STATUS_REJECT_SHORT:
                    fprintf(stderr, "Julian: <input rejected by short input>\n");
                    break;

                case J_RESULT_STATUS_FAIL:
                    fprintf(stderr, "Julian: <search failed>\n");
                    break;
            }
            /* continue to next process instance */
            continue;
        }

        /* output results for all the obtained sentences */
        winfo = r->lm->winfo;

        for (n = 0; n < r->result.sentnum; n++) {         /* for all sentences */
            has_data = true;
            s        = &(r->result.sent[n]);
            seq      = s->word;
            seqnum   = s->word_num;
            if (!seqnum)
                continue;

            /* output word sequence like Julius */
            Sentences += (char *)"<ph>\n\t";
            for (i = 0; i < seqnum; i++) {
                if (i)
                    Sentences += (char *)" ";
                Sentences += (char *)winfo->woutput[seq[i]];
            }
            Sentences += (char *)"\n";
            /* LM entry sequence */
            Sentences += (char *)"\t<d wsek=\"";
            for (i = 0; i < seqnum; i++) {
                if (i)
                    Sentences += (char *)" ";
                Sentences += winfo->wname[seq[i]];
            }

            /* phoneme sequence */
            Sentences += (char *)"\" phseq=\"";
            Sentences += put_hypo_phoneme(seq, seqnum, winfo);
            /* confidence scores */
            Sentences += (char *)"\" cmscore=\"";
            for (i = 0; i < seqnum; i++) {
                if (i)
                    Sentences += (char *)" ";
                Sentences += AnsiString((double)s->confidence[i]);
            }
            Sentences += (char *)"\"/>\n";
            Sentences += (char *)"</ph>\n";
        }
    }
    if ((has_data) && (jo->on_sentece)) {
        Sentences += (char *)"</sl>\n";
        RemoteInvoker(jo, INVOKE_FIREEVENT, (int)EVENT_ON_SENTENCE, Sentences.c_str());
    }
}

#define LOAD_PARAM(_param_)                                        if (_param_.Length()) { argv[argc++] = _param_.c_str(); }
#define LOAD_PARAM2(_prefix_, _param_)                             if (_param_.Length()) { argv[argc++] = _prefix_; argv[argc++] = _param_.c_str(); }

#define LOAD_PARAM_BOOL(_prefix_, _param_)                         if (_param_.ToInt()) { argv[argc++] = _prefix_; }
#define LOAD_PARAM_BOOL2(_prefix_, _prefix_no_, _param_)           if (_param_.ToInt()) { argv[argc++] = _prefix_; } else { argv[argc++] = _prefix_no_; }

#define LOAD_PARAM_BOOL2_IF_SET(_prefix_, _prefix_no_, _param_)    if (_param_.Length()) { if (_param_.ToInt() == 1) { argv[argc++] = _prefix_; } else { argv[argc++] = _prefix_no_; } }

THREAD_TYPE RecognitionThread(LPVOID SENDER) {
    JuliusObject *jo = (JuliusObject *)SENDER;
    char         *argv[MAX_PARAMS];
    int          argc = 0;

    argv[argc++] = "conclient";
    argv[argc++] = "-input";
    argv[argc++] = "mic";
    argv[argc++] = "-forcedict";
    argv[argc++] = "-notypecheck";
    LOAD_PARAM2("-C", jo->ConfFile);
    LOAD_PARAM2("-dfa", jo->DFAGram);
    LOAD_PARAM2("-v", jo->DictFile);
    LOAD_PARAM2("-h", jo->HMMDefs);
    LOAD_PARAM2("-hlsit", jo->HMMList);
    LOAD_PARAM2("-penalty1", jo->Penalty1);
    LOAD_PARAM2("-penalty2", jo->Penalty2);
    LOAD_PARAM_BOOL2_IF_SET("-no-ccd", "-force-ccd", jo->CCD);
    LOAD_PARAM2("-iwcd1", jo->IWCD1);
    LOAD_PARAM2("-tmix", jo->Mixtures);
    LOAD_PARAM2("-gprune", jo->Prunning);
    LOAD_PARAM2("-gshmm", jo->GSHMM);
    LOAD_PARAM2("-gsnum", jo->GSNum);
    LOAD_PARAM2("-b", jo->BeamWidth);
    LOAD_PARAM2("-b2", jo->BeamWidth2);
    LOAD_PARAM2("-sb", jo->ScoreBeam);
    LOAD_PARAM2("-s", jo->HypothesesStack);
    LOAD_PARAM2("-m", jo->HypothesesOverflow);
    LOAD_PARAM2("-lookuprange", jo->LookupRange);
    LOAD_PARAM2("-n", jo->Sentences);
    LOAD_PARAM2("-output", jo->Output);
    LOAD_PARAM2("-looktrellis", jo->LookTrellis);
    LOAD_PARAM2("-spmodel", jo->SPModel);
    LOAD_PARAM2("-iwsp", jo->iwsp);
    LOAD_PARAM2("-iwsppenalty", jo->iwsppenalty);
    LOAD_PARAM2("-gmm", jo->GMM);
    LOAD_PARAM2("-gmmnum", jo->GMMNum);
    LOAD_PARAM2("-gmmreject", jo->GMMReject);
    LOAD_PARAM2("-rejectshor", jo->RejectShort);
    LOAD_PARAM_BOOL2_IF_SET("-pausesegment", "-nopausesegment", jo->PauseSegment);
    LOAD_PARAM2("-lv", jo->TresholdLevel);
    LOAD_PARAM2("-headmargin", jo->HeadMargin);
    LOAD_PARAM2("-tailmargin", jo->TailMargin);
    LOAD_PARAM2("-zc", jo->ZeroCross);
    LOAD_PARAM2("-smpFreq", jo->SampleRate);
    LOAD_PARAM2("-smpPeriod", jo->SamplePeriod);
    LOAD_PARAM2("-fsize", jo->WindowSize);
    LOAD_PARAM2("-fshift", jo->FrameShift);
    LOAD_PARAM2("-delwin", jo->DeltaWindow);
    LOAD_PARAM2("-hifreq", jo->HiFreq);
    LOAD_PARAM2("-lofreq", jo->LoFreq);
    LOAD_PARAM2("-walign", jo->WAlign);
    LOAD_PARAM2("-palign", jo->PAlign);
    LOAD_PARAM2("-salign", jo->SAlign);
    LOAD_PARAM2("-cmalpha", jo->CMAlpha);
    LOAD_PARAM_BOOL("-separatescore", jo->SeparateScore);
    LOAD_PARAM_BOOL("-progout", jo->ProgOut);
    LOAD_PARAM2("-proginterval", jo->ProgInterval);
    LOAD_PARAM_BOOL("-quiet", jo->Quiet);
    LOAD_PARAM_BOOL("-demo", jo->Demo);
    LOAD_PARAM_BOOL("-debug", jo->Debug);
    LOAD_PARAM_BOOL("-module", jo->Module);
    LOAD_PARAM2("-outcode", jo->OutCode);
    LOAD_PARAM2("-gram", jo->GramFile);
    LOAD_PARAM2("-gramlist", jo->GramList);
    Jconf *jconf = j_config_load_args_new(argc, argv);
    if (!jconf) {
        fprintf(stderr, "Julius: jconf error\n");
        return 0;
    }
    jo->recog = j_create_instance_from_jconf(jconf);
    if (!jo->recog) {
        fprintf(stderr, "Julius: Error in startup\n");
        return 0;
    }

    callback_add(jo->recog, CALLBACK_EVENT_SPEECH_READY, status_recready, jo);
    callback_add(jo->recog, CALLBACK_EVENT_SPEECH_START, status_recstart, jo);
    callback_add(jo->recog, CALLBACK_RESULT, output_result, jo);
    jo->MicError = false;
    if (j_adin_init(jo->recog) == FALSE) {
        jo->MicError = true;
        j_recog_free(jo->recog);
        jo->recog = 0;
        fprintf(stderr, "Julius: error initializing microphone\n");
        return 0;
    }
    bool stream_ok = false;
    switch (j_open_stream(jo->recog, NULL)) {
        case 0:
            stream_ok = true;
            break;

        case -1:
            fprintf(stderr, "error in input stream\n");
            j_recog_free(jo->recog);
            jo->recog = 0;
            return 0;

        case -2:
            fprintf(stderr, "failed to begin input stream\n");
            j_recog_free(jo->recog);
            jo->recog = 0;
            return 0;
    }

    int ret = j_recognize_stream(jo->recog);
    if (ret == -1)
        stream_ok = false;

    if (jconf) {
        j_jconf_free(jconf);
        jconf = 0;
    }
    if (stream_ok)
        j_close_stream(jo->recog);
    j_recog_free(jo->recog);
    jo->recog = 0;
    return 0;
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

                JuliusObject *ctx = (JuliusObject *)context;
                switch (event_id) {
                    case EVENT_ON_SENTENCE:
                        ctx->on_sentece = true;
                        return E_ACCEPTED;
                        break;

                    case EVENT_ON_RECSTART:
                        ctx->on_recstart = true;
                        return E_ACCEPTED;
                        break;

                    case EVENT_ON_RECREADY:
                        ctx->on_recready = true;
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

                JuliusObject *ctx = (JuliusObject *)context;
                switch (prop_id) {
                    case P_STARTRECOGNITION:
                        if (param.ToInt())
                            ctx->Run();
                        break;

                    case P_PAUSERECOGNITION:
                        ctx->Pause(param.ToInt());
                        break;

                    case P_TERMINATERECOGNITION:
                        if (param.ToInt())
                            ctx->Terminate();
                        break;

                    case P_ConfFile:
                        ctx->ConfFile = param;
                        break;

                    case P_DFAGram:
                        ctx->DFAGram = param;
                        break;

                    case P_DictFile:
                        ctx->DictFile = param;
                        break;

                    case P_HMMDefs:
                        ctx->HMMDefs = param;
                        break;

                    case P_HMMList:
                        ctx->HMMList = param;
                        break;

                    case P_Penalty1:
                        ctx->Penalty1 = param;
                        break;

                    case P_Penalty2:
                        ctx->Penalty2 = param;
                        break;

                    case P_CCD:
                        ctx->CCD = param;
                        break;

                    case P_IWCD1:
                        ctx->IWCD1 = param;
                        break;

                    case P_Mixtures:
                        ctx->Mixtures = param;
                        break;

                    case P_Prunning:
                        ctx->Prunning = param;
                        break;

                    case P_GSHMM:
                        ctx->GSHMM = param;
                        break;

                    case P_GSNum:
                        ctx->GSNum = param;
                        break;

                    case P_BeamWidth:
                        ctx->BeamWidth = param;
                        break;

                    case P_BeamWidth2:
                        ctx->BeamWidth2 = param;
                        break;

                    case P_ScoreBeam:
                        ctx->ScoreBeam = param;
                        break;

                    case P_HypothesesStack:
                        ctx->HypothesesStack = param;
                        break;

                    case P_HypothesesOverflow:
                        ctx->HypothesesOverflow = param;
                        break;

                    case P_LookupRange:
                        ctx->LookupRange = param;
                        break;

                    case P_Sentences:
                        ctx->Sentences = param;
                        break;

                    case P_Output:
                        ctx->Output = param;
                        break;

                    case P_LookTrellis:
                        ctx->LookTrellis = param;
                        break;

                    case P_SPModel:
                        ctx->SPModel = param;
                        break;

                    case P_iwsp:
                        ctx->iwsp = param;
                        break;

                    case P_iwsppenalty:
                        ctx->iwsppenalty = param;
                        break;

                    case P_GMM:
                        ctx->GMM = param;
                        break;

                    case P_GMMNum:
                        ctx->GMMNum = param;
                        break;

                    case P_GMMReject:
                        ctx->GMMReject = param;
                        break;

                    case P_RejectShort:
                        ctx->RejectShort = param;
                        break;

                    case P_PauseSegment:
                        ctx->PauseSegment = param;
                        break;

                    case P_TresholdLevel:
                        ctx->TresholdLevel = param;
                        break;

                    case P_HeadMargin:
                        ctx->HeadMargin = param;
                        break;

                    case P_TailMargin:
                        ctx->TailMargin = param;
                        break;

                    case P_ZeroCross:
                        ctx->ZeroCross = param;
                        break;

                    case P_SampleRate:
                        ctx->SampleRate = param;
                        break;

                    case P_SamplePeriod:
                        ctx->SamplePeriod = param;
                        break;

                    case P_WindowSize:
                        ctx->WindowSize = param;
                        break;

                    case P_FrameShift:
                        ctx->FrameShift = param;
                        break;

                    case P_DeltaWindow:
                        ctx->DeltaWindow = param;
                        break;

                    case P_HiFreq:
                        ctx->HiFreq = param;
                        break;

                    case P_LoFreq:
                        ctx->LoFreq = param;
                        break;

                    case P_WAlign:
                        ctx->WAlign = param;
                        break;

                    case P_PAlign:
                        ctx->PAlign = param;
                        break;

                    case P_SAlign:
                        ctx->SAlign = param;
                        break;

                    case P_CMAlpha:
                        ctx->CMAlpha = param;
                        break;

                    case P_SeparateScore:
                        ctx->SeparateScore = param;
                        break;

                    case P_ProgOut:
                        ctx->ProgOut = param;
                        break;

                    case P_ProgInterval:
                        ctx->ProgInterval = param;
                        break;

                    case P_Quiet:
                        ctx->Quiet = param;
                        break;

                    case P_Demo:
                        ctx->Demo = param;
                        break;

                    case P_Debug:
                        ctx->Debug = param;
                        break;

                    case P_Module:
                        ctx->Module = param;
                        break;

                    case P_OutCode:
                        ctx->OutCode = param;
                        break;

                    case P_GramFile:
                        ctx->GramFile = param;
                        break;

                    case P_GramList:
                        ctx->GramList = param;
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
                JuliusObject *ctx = (JuliusObject *)context;
                switch (prop_id) {
                    case P_RUNNING:
                        if (ctx->recog)
                            buffered_text = (char *)"1";
                        else
                            buffered_text = (char *)"0";
                        break;

                    case P_MICERROR:
                        if (ctx->MicError)
                            buffered_text = (char *)"1";
                        else
                            buffered_text = (char *)"0";
                        break;
                }

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
                JuliusObject *ctx = new JuliusObject();

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
            return E_NOERROR;

        case INVOKE_DONELIBRARY:
            return E_NOERROR;

        default:
            return E_NOTIMPLEMENTED;
    }
    return E_NOTIMPLEMENTED;
}
