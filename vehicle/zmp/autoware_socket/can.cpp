/*
 *  Copyright (c) 2015, Nagoya University
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Autoware nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "mainwindow.h"
#include "autoware_socket.h"

// CAN + Drive Mode
std::string candata;
int drvmode;

void *CANSenderEntry(void *a)
{
  struct sockaddr_in server;
  int sock;
  std::string senddata; 
  std::ostringstream oss;
  oss << candata << "," << drvmode; // global variables.
  senddata = oss.str();
  unsigned int **addrptr;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("socket");
    return NULL;
  }

  server.sin_family = AF_INET;
  server.sin_port = htons(10000);

  server.sin_addr.s_addr = inet_addr(ros_ip_address.c_str());
  if (server.sin_addr.s_addr == 0xffffffff) {
    struct hostent *host;

    host = gethostbyname(ros_ip_address.c_str());
    if (host == NULL) {
      if (h_errno == HOST_NOT_FOUND) {
        fprintf(stderr,"info : host not found : %s\n", ros_ip_address.c_str());
      } else {
        fprintf(stderr,"info : %s : %s\n", hstrerror(h_errno), ros_ip_address.c_str());
      }
      return NULL;
    }

    addrptr = (unsigned int **)host->h_addr_list;

    while (*addrptr != NULL) {
      server.sin_addr.s_addr = *(*addrptr);
      
      // if connected, break out this loop.
      if (connect(sock,
                  (struct sockaddr *)&server,
                  sizeof(server)) == 0) {
        break;
      }

      addrptr++;
      // if failed to connect, try another address.
    }

    // if totally failed to connect, return NULL.
    if (*addrptr == NULL) {
      perror("info : connect");
      return NULL;
    }
  } else {
    if (connect(sock,
		(struct sockaddr *)&server, sizeof(server)) != 0) {
      perror("info : connect");
      return NULL;
    }
  }

  int n;

  printf("info : %s\n",senddata.c_str());
    
  n = send(sock, senddata.c_str(), senddata.size()+1,0);
  if (n < 0) {
    perror("write");
    return NULL;
  }
    
  close(sock);

  return NULL;
}

void wrapSender(void)
{
  pthread_t _cansender;
  
  if(pthread_create(&_cansender, NULL, CANSenderEntry, NULL)){
    fprintf(stderr,"info : pthread create error");
    return;
  }

  usleep(can_tx_interval*1000);
  pthread_join(_cansender, NULL);
}

void MainWindow::SendCAN(void)
{
  char tmp[300] = "";
  string can = "";

  // add time when sent out.
  sprintf(tmp,"'%d/%02d/%02d %02d:%02d:%02d.%ld',",
          _s_time->tm_year+1900, _s_time->tm_mon+1, _s_time->tm_mday,
          _s_time->tm_hour + 9, _s_time->tm_min, _s_time->tm_sec, 
          _getTime.tv_usec);

  can += tmp;

#if 0    
  if(_selectLog.drvInf == true){
    sprintf(tmp,"%d,%d,%d,%d,%d,%d,%d,%3.2f,%3.2f,%d,%d,%d,",
            _drvInf.mode, _drvInf.contMode, _drvInf.overrideMode, 
            _drvInf.servo, _drvInf.actualPedalStr, _drvInf.targetPedalStr, 
            _drvInf.inputPedalStr,
            _drvInf.targetVeloc, _drvInf.veloc,
            _drvInf.actualShift, _drvInf.targetShift, _drvInf.inputShift);
    can += tmp;
  } else {
    sprintf(tmp,"0,0,0,0,0,0,0,0,0,0,0,0,");
    can += tmp;
  }
  if(_selectLog.strInf == true){
    sprintf(tmp,"%d,%d,%d,%d,%d,%d,%3.2f,%3.2f,",
            _strInf.mode, _strInf.cont_mode, _strInf.overrideMode, 
            _strInf.servo,
            _strInf.targetTorque, _strInf.torque,
            _strInf.angle, _strInf.targetAngle);
    can += tmp;
  } else {
    sprintf(tmp,"0,0,0,0,0,0,0,0,");
    can += tmp;
  }
  if(_selectLog.brkInf == true){
    sprintf(tmp,"%d,%d,%d,%d,",
            _brakeInf.pressed, _brakeInf.actualPedalStr, 
            _brakeInf.targetPedalStr, _brakeInf.inputPedalStr);
    can += tmp;
  } else {
    sprintf(tmp,"0,0,0,0,");
    can += tmp;
  }
  if(_selectLog.battInf == true){
    sprintf(tmp,"%3.2f,%d,%3.2f,%d,%d,%3.2f,%3.2f,",
            _battInf.soc, _battInf.voltage, _battInf.current,
            _battInf.max_temp, _battInf.min_temp,
            _battInf.max_chg_current, _battInf.max_dischg_current);
    can += tmp;
  } else {
    sprintf(tmp,"0,0,0,0,0,0,0,");
    can += tmp;
  }
  if(_selectLog.otherInf == true){
    sprintf(tmp,"%3.3f,%3.5f,%3.1f,%3.1f,%3.1f,%3.1f,%3.1f,%3.1f,%3.1f,%d,%d,%d,%3.1f,%d,%d,%d,%d,%d,%d,%d",
            _otherInf.sideAcc, _otherInf.acc, _otherInf.angleFromP, 
            _otherInf.brkPedalStrFromP, _otherInf.velocFrFromP, 
            _otherInf.velocFlFromP, _otherInf.velocRrFromP, 
            _otherInf.velocRlFromP,
            _otherInf.velocFromP2,
            _otherInf.drv_mode, _otherInf.drvPedalStrFromP, _otherInf.rpm,
            _otherInf.velocFlFromP,
            _otherInf.ev_mode, _otherInf.temp, _otherInf.shiftFromPrius, 
            _otherInf.light,
            _otherInf.level, _otherInf.door, _otherInf.cluise);
    
    can += tmp;
  } else {
    sprintf(tmp,"0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
    can += tmp;
  }
#endif
  
  //sprintf(tmp,"\n");
  candata = can;

  // send drive mode in addition to CAN.
  UpdateState();
  if (ZMP_DRV_CONTROLLED() && ZMP_STR_CONTROLLED()) {
    drvmode = CMD_MODE_PROGRAM;
  } else {
    drvmode = CMD_MODE_MANUAL;
  }
  
  wrapSender();
}






