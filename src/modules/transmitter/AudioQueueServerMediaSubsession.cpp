/*
 *  AudioQueueServerMediaSubsession.hh - It consumes audio frames from the frame queue on demand
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of liveMediaStreamer.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Authors:  David Cassany <david.cassany@i2cat.net>,
 *            
 */

#include "AudioQueueServerMediaSubsession.hh"
#include "QueueSource.hh"
#include "../../AVFramedQueue.hh"


AudioQueueServerMediaSubsession*
AudioQueueServerMediaSubsession::createNew(Connection* conn, UsageEnvironment& env,
                          StreamReplicator* replica, int readerId,
                          ACodecType codec, unsigned channels,
                          unsigned sampleRate, SampleFmt sampleFormat,
                          Boolean reuseFirstSource) {
  return new AudioQueueServerMediaSubsession(conn, env, replica, readerId, 
                                             codec, channels, sampleRate,
                                             sampleFormat, reuseFirstSource);
}

AudioQueueServerMediaSubsession::AudioQueueServerMediaSubsession(Connection* conn, UsageEnvironment& env,
                          StreamReplicator* replica, int readerId, 
                          ACodecType codec, unsigned channels,
                          unsigned sampleRate, SampleFmt sampleFormat,
                          Boolean reuseFirstSource)
  : QueueServerMediaSubsession(env, reuseFirstSource), replicator(replica), reader(readerId), fCodec(codec),
                          fChannels(channels), fSampleRate(sampleRate), fSampleFormat(sampleFormat), fConn(conn)
{
}

AudioQueueServerMediaSubsession::~AudioQueueServerMediaSubsession()
{
}

FramedSource* AudioQueueServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
    
    unsigned bitsPerSecond = 0;
    
    if (fSampleFormat == S16){
        bitsPerSecond = fSampleRate*2*fChannels;
    } else if (fSampleFormat == U8){
        bitsPerSecond = fSampleRate*1*fChannels;
    }
    
    if (bitsPerSecond > 0){
        estBitrate = (bitsPerSecond+500)/1000; // kbps
    } else {
        estBitrate = 128; // kbps, estimate
    }
    return replicator->createStreamReplica();
}

RTPSink* AudioQueueServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
           unsigned char rtpPayloadTypeIfDynamic,
           FramedSource* /*inputSource*/) {
    unsigned char payloadType = rtpPayloadTypeIfDynamic;
    
    std::string codecStr = utils::getAudioCodecAsString(fCodec);
    
    if (fCodec == PCM && 
        fSampleFormat == S16) {
        codecStr = "L16";
        if (fSampleRate == 44100) {
            if (fChannels == 2){
                payloadType = 10;
            } else if (fChannels == 1) {
                payloadType = 11;
            }
        }
    } else if ((fCodec == PCMU || fCodec == G711) && 
                fSampleRate == 8000 &&
                fChannels == 1){
        payloadType = 0;
    }

     if (fCodec == MP3){
        return MPEG1or2AudioRTPSink::createNew(envir(), rtpGroupsock);
    }

    return SimpleRTPSink
    ::createNew(envir(), rtpGroupsock, payloadType,
                fSampleRate, "audio", 
                codecStr.c_str(),
                fChannels, False);
}

RTCPInstance* AudioQueueServerMediaSubsession::createRTCP(Groupsock* RTCPgs, unsigned totSessionBW, /* in kbps */
                   unsigned char const* cname, RTPSink* sink)
{
    //TODO: reach setting id as the RTP port (as done for RTPConnection)
    size_t id = rand();

    ConnRTCPInstance* newRTCPInstance = ConnRTCPInstance::createNew(fConn, &envir(), RTCPgs, totSessionBW, sink);
    newRTCPInstance->setId(id);
    fConn->addConnectionRTCPInstance(id, newRTCPInstance);

    return newRTCPInstance;
}

std::vector<int> AudioQueueServerMediaSubsession::getReaderIds()
{
    std::vector<int> readers;
    readers.push_back(reader);
    return readers;
}

