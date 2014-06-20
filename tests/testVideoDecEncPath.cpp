#ifndef _LIVEMEDIA_HH
#include <liveMedia/liveMedia.hh>
#endif

#include "../src/modules/liveMediaInput/SourceManager.hh"
#include "../src/Utils.hh"
#include "../src/Controller.hh"
#include "../src/Callbacks.hh"

#ifndef _VIDEO_DECODER_LIBAV_HH
#include "../src/modules/videoDecoder/VideoDecoderLibav.hh"
#endif

#ifndef _FRAME_QUEUE_HH
#include "../src/FrameQueue.hh"
#endif

#ifndef _INTERLEAVED_VIDEO_FRAME_HH
#include "../src/VideoFrame.hh"
#endif

#ifndef _VIDEO_ENCODER_X264_HH
#include "../src/modules/videoEncoder/VideoEncoderX264.hh"
#endif

#include <string>
#include <fstream>
#include <iostream>
#include <csignal>

#define V_MEDIUM "video"
#define PROTOCOL "RTP"
#define PAYLOAD 96
#define V_CODEC "H264"
#define BANDWITH 5000
#define V_CLIENT_PORT 6004
#define V_TIME_STMP_FREQ 90000

#define A_CODEC "AC3"
#define A_CLIENT_PORT 6006
#define A_MEDIUM "audio"
#define A_TIME_STMP_FREQ 44100


void signalHandler(int signum)
{
    utils::infoMsg("Interruption signal received");
    
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    pipe->stopWorkers();
    
    utils::infoMsg("Workers Stopped");
	exit(1);
}

void connect(char const* medium, unsigned short port){}

int main(int argc, char** argv) 
{   
    std::string sessionId, sessionIdTransmitter, sessionIdTransmitterSlave1, sessionIdTransmitterSlave2;
    std::string sdp;
    Session* session;
	PipelineManager *pipeMngr = Controller::getInstance()->pipelineManager();
	SourceManager *mngr = pipeMngr->getReceiver();
	SinkManager *transmitter = pipeMngr->getTransmitter(); 
    //SourceManager *mngr = SourceManager::getInstance();
    FrameQueue* queue;
   	//Frame* rawFrame = InterleavedVideoFrame::createNew(RAW, DEFAULT_WIDTH, DEFAULT_HEIGHT, RGB24);
	Frame* h264Frame1080 = InterleavedVideoFrame::createNew(H264, 1920, 1080, YUYV422);
	Frame* h264Frame720 = InterleavedVideoFrame::createNew(H264, 1280, 720, YUYV422);
	Frame* h264Frame480 = InterleavedVideoFrame::createNew(H264, 720, 480, YUYV422);
    VideoDecoderLibav* decoder = new VideoDecoderLibav();
	//VideoEncoderX264* encoder = new VideoEncoderX264();
	VideoEncoderX264* encoder720 = new VideoEncoderX264();
	VideoEncoderX264* encoder480 = new VideoEncoderX264();
	VideoEncoderX264* encoder1080 = new VideoEncoderX264();
    Worker *vDecoderWorker = new Worker();
	Worker *vEncoderWorker;// = new Worker();
	Master *vEncoderMaster = new Master();
	Slave *vEncoderSlave1 = new Slave();
	Slave *vEncoderSlave2 = new Slave();
	Slave *vEncoderSlave3 = new Slave();
    std::ofstream h264Frames720, h264Frames480, h264Frames1080;

	encoder720->configure(1280, 720, YUYV422);
	encoder480->configure(720, 480, YUYV422);
	//encoder1080->configure(1920, 1080, YUYV422);

	//mngr->setCallback((void(*)(char const*, unsigned short))&connect);
	mngr->setCallback(callbacks::connectToDecoderCallback);

    
    //condif decoder

	int pathID = V_CLIENT_PORT;
	int pathMaster = rand();
	int pathSlave1 = rand();
	int pathSlave2 = rand();
	int videoDecoderID = rand();
	int videoEncoderMasterID;
	int videoEncoderSlave1ID;
	int videoEncoderSlave2ID;
	//int id1 = V_CLIENT_PORT;
    
    signal(SIGINT, signalHandler); 
    
    for (int i = 1; i <= argc-1; ++i) {
        sessionId = utils::randomIdGenerator(ID_LENGTH);
        session = Session::createNewByURL(*(mngr->envir()), argv[0], argv[i], sessionId);
        mngr->addSession(session);
		session->initiateSession();
    }
	
	pipeMngr->startWorkers();
    
   /* sessionId = utils::randomIdGenerator(ID_LENGTH);
    
    sdp = SourceManager::makeSessionSDP(sessionId, "this is a test");
    
    sdp += SourceManager::makeSubsessionSDP(V_MEDIUM, PROTOCOL, PAYLOAD, V_CODEC, 
                                       BANDWITH, V_TIME_STMP_FREQ, V_CLIENT_PORT);*/
    // sdp += handlers::makeSubsessionSDP(A_MEDIUM, PROTOCOL, PAYLOAD, A_CODEC, 
    //                                    BANDWITH, A_TIME_STMP_FREQ, A_CLIENT_PORT);
    
    //session = Session::createNew(*(mngr->envir()), sdp, sessionId);
    
    //mngr->addSession(session);
    //session->initiateSession();
    
    //mngr->runManager();
	//transmitter->runManager();
	
	sleep(3);
	Path* decoderPath;//= pipeMngr->getPath(pathID);
	std::map<int, Path*> paths = pipeMngr->getPaths();
	for (auto it : paths) {
		decoderPath = it.second;
		break;
	}


	if(!pipeMngr->addFilter(videoDecoderID, decoder)) {
       utils::errorMsg("Error adding decoder to the pipeline");
    }

	if(!pipeMngr->addWorker(videoDecoderID, vDecoderWorker)) {
        utils::errorMsg("Error adding decoder worker");
    }
	
	decoderPath->setDestinationFilter(videoDecoderID, pipeMngr->getFilter(videoDecoderID)->generateReaderID());
	
	pipeMngr->connectPath(decoderPath);
       
    //Let some time to initiate reciver sessions
    sleep(2);

     //mngr->getWriterID(V_CLIENT_PORT);

    //if(!mngr->connect(id1, decoder, decoder->getAvailableReaders().front())) {
	/*if(!mngr->connectManyToOne(decoder, id1)) {
        std::cerr << "Error connecting video decoder" << std::endl;
    }*/
	
	//int id2 = decoder->getAvailableWriters().front();
	//if(!decoder->connect(id2, encoder, encoder->getAvailableReaders().front())) {
//	if(!decoder->connectOneToOne(encoder720)) {
	//vDecoderWorker = new Worker();

	/*if(!decoder->connectOneToOne(encoder)) {
        std::cerr << "Error connecting video encoder" << std::endl;
    }*/
	utils::debugMsg("START Master Path");
	Path* encoderPathMaster = new VideoEncoderPath(videoDecoderID, decoder->generateWriterID());
	std::vector<int> filters = encoderPathMaster->getFilters();
	videoEncoderMasterID = filters[0];

	if(!pipeMngr->addWorker(videoEncoderMasterID, vEncoderMaster)) {
        utils::errorMsg("Error adding decoder worker");
    }	

	encoderPathMaster->setDestinationFilter(pipeMngr->getTransmitterID(), transmitter->generateReaderID());

	if (!pipeMngr->addPath(pathMaster, encoderPathMaster)) {
        utils::errorMsg("Error adding master path to the pipeline");
    }

	if (!pipeMngr->connectPath(encoderPathMaster)) {
		utils::errorMsg("Error connecting master path");
	}

    std::vector<int> readers;
    readers.push_back(encoderPathMaster->getDstReaderID());

    sessionIdTransmitter = utils::randomIdGenerator(ID_LENGTH);
    if (!transmitter->addSession(sessionIdTransmitter, readers)) {
        return 1;
    }

    transmitter->publishSession(sessionIdTransmitter);
	utils::debugMsg("END Master Path");

	utils::debugMsg("START Slave Path 1");
	Path* encoderPathSlave1 = new VideoEncoderPath(videoDecoderID, decoder->generateWriterID(), true);
	std::vector<int> filtersSlave1 = encoderPathSlave1->getFilters();
	videoEncoderSlave1ID = filtersSlave1[0];
	encoder720 = dynamic_cast<VideoEncoderX264*> (pipeMngr->getFilter(videoEncoderSlave1ID));
	encoder720->configure(1280, 720, YUYV422);

	if(!pipeMngr->addWorker(videoEncoderSlave1ID, vEncoderSlave1)) {
        utils::errorMsg("Error adding decoder worker");
    }	

	encoderPathSlave1->setDestinationFilter(pipeMngr->getTransmitterID(), transmitter->generateReaderID());

	if (!pipeMngr->addPath(pathSlave1, encoderPathSlave1)) {
        utils::errorMsg("Error adding slave1 path to the pipeline");
    }

	if (!pipeMngr->connectPath(encoderPathSlave1)) {
		utils::errorMsg("Error connecting slave1 path");
	}

    std::vector<int> readersSlave1;
    readersSlave1.push_back(encoderPathSlave1->getDstReaderID());

    sessionIdTransmitterSlave1 = utils::randomIdGenerator(ID_LENGTH);
    if (!transmitter->addSession(sessionIdTransmitterSlave1, readersSlave1)) {
        return 1;
    }

    transmitter->publishSession(sessionIdTransmitterSlave1);
	utils::debugMsg("END Slave Path 1");

	utils::debugMsg("START Slave Path 2");
	Path* encoderPathSlave2 = new VideoEncoderPath(videoDecoderID, decoder->generateWriterID(), true);
	std::vector<int> filtersSlave2 = encoderPathSlave2->getFilters();
	videoEncoderSlave2ID = filtersSlave2[0];
	encoder480 = dynamic_cast<VideoEncoderX264*> (pipeMngr->getFilter(videoEncoderSlave2ID));
	encoder480->configure(720, 480, YUYV422);

	if(!pipeMngr->addWorker(videoEncoderSlave2ID, vEncoderSlave2)) {
        utils::errorMsg("Error adding decoder worker");
    }	

	encoderPathSlave2->setDestinationFilter(pipeMngr->getTransmitterID(), transmitter->generateReaderID());

	if (!pipeMngr->addPath(pathSlave2, encoderPathSlave2)) {
        utils::errorMsg("Error adding slave2 path to the pipeline");
    }

	if (!pipeMngr->connectPath(encoderPathSlave2)) {
		utils::errorMsg("Error connecting slave2 path");
	}

    std::vector<int> readersSlave2;
    readersSlave2.push_back(encoderPathSlave2->getDstReaderID());

    sessionIdTransmitterSlave2 = utils::randomIdGenerator(ID_LENGTH);
    if (!transmitter->addSession(sessionIdTransmitterSlave2, readersSlave2)) {
        return 1;
    }

    transmitter->publishSession(sessionIdTransmitterSlave2);
	utils::debugMsg("END Slave Path 2");

	//vEnconderMaster = new Master();
/*    if(!pipeMngr->addFilter(videoEncoderMasterID, encoder)) {
        std::cerr << "Error adding encoder to the pipeline" << std::endl;
    }*/
/*
	if(!pipeMngr->addWorker(videoEncoderMasterID, vEnconderMaster)) {
        std::cerr << "Error adding encoder worker" << std::endl;
    }*/

	/*if(!decoder->connectOneToOne(encoder720, true)) {
        std::cerr << "Error connecting video encoder!" << std::endl;
    }

	if(!decoder->connectOneToOne(encoder480, true)) {
        std::cerr << "Error connecting video encoder!!" << std::endl;
    }*/

   // Reader *reader1080 = new Reader();
	//BaseFilter *encoder = pipeMngr->getFilter(videoEncoderMasterID);
	//encoder->connect(reader1080);
  /*  Reader *reader720 = new Reader();
	encoder720->connect(reader720);
	Reader *reader480 = new Reader();
	encoder480->connect(reader480);*/
	//Reader *reader1080 = new Reader();
	//encoder1080->connect(reader1080);

    //vDecoderWorker = new Worker(decoder);
	//vEnconderMaster = new Master(encoder);
	//vEncoderWorker = new Worker(encoder);
	
	/*vEncoderSlave1 = new Slave(1, encoder720);
	vEnconderMaster->addSlave(vEncoderSlave1);
	vEncoderSlave2 = new Slave(2, encoder480);
	vEnconderMaster->addSlave(vEncoderSlave2);*/
	//vEncoderSlave3 = new Slave(3, encoder1080);
	//vEnconderMaster->addSlave(vEncoderSlave3);
	
	vEncoderMaster->addSlave(vEncoderSlave1);
	vEncoderMaster->addSlave(vEncoderSlave2);
    vDecoderWorker->start();
	vEncoderMaster->setFps(24);
	vEncoderSlave1->setFps(24);
	vEncoderSlave2->setFps(24);
	//vEncoderWorker->start();
	vEncoderMaster->start();
	vEncoderSlave1->start();
	vEncoderSlave2->start();
	//vEncoderSlave3->start();
    while(pipeMngr->getWorker(pipeMngr->getReceiverID())->isRunning() || 
        pipeMngr->getWorker(pipeMngr->getTransmitterID())->isRunning()) {
        sleep(1);
    }
    
   // while(mngr->isRunning()) {
		//printf("antes getFrame\n");
		/*h264Frame720 = reader720->getFrame();
		h264Frame480 = reader480->getFrame();
		//h264Frame1080 = reader1080->getFrame();
		
        //if (!h264Frame720 || !h264Frame1080 || !h264Frame480) {
		if (!h264Frame720 || !h264Frame480) {
            usleep(500);
            continue;
        }
		//printf("despues getFrame\n");
        if (! h264Frames720.is_open()){
            h264Frames720.open("frames720.h264", std::ios::out | std::ios::app | std::ios::binary);
        }

        if (! h264Frames480.is_open()){
            h264Frames480.open("frames480.h264", std::ios::out | std::ios::app | std::ios::binary);
        }*/
/*
        if (! h264Frames1080.is_open()){
            h264Frames1080.open("frames1080.h264", std::ios::out | std::ios::app | std::ios::binary);
        }*/
		
	/*	if (h264Frame720->getLength() > 0) {
            h264Frames720.write(reinterpret_cast<const char*>(h264Frame720->getDataBuf()), h264Frame720->getLength());
            //printf("Filled buffer! Frame size: %d\n", h264Frame->getLength());
        }

		if (h264Frame480->getLength() > 0) {
            h264Frames480.write(reinterpret_cast<const char*>(h264Frame480->getDataBuf()), h264Frame480->getLength());
            //printf("Filled buffer! Frame size: %d\n", h264Frame->getLength());
        }
        */
	/*	if (h264Frame1080->getLength() > 0) {
            h264Frames1080.write(reinterpret_cast<const char*>(h264Frame1080->getDataBuf()), h264Frame1080->getLength());
            //printf("Filled buffer! Frame size: %d\n", h264Frame->getLength());
        }*/

      /*  reader720->removeFrame();        
        reader480->removeFrame();*/
		//reader1080->removeFrame();
    //}
    
   // h264Frames720.close();
	//h264Frames480.close();
	//h264Frames1080.close();
    
    return 0;
}
