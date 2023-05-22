/*
 * Copyright (C) 2018 dimercur
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tasks.h"
#include <stdexcept>

// Déclaration des priorités des taches
#define PRIORITY_TSERVER 30
#define PRIORITY_TOPENCOMROBOT 20
#define PRIORITY_TMOVE 20
#define PRIORITY_TSENDTOMON 22
#define PRIORITY_TRECEIVEFROMMON 25
#define PRIORITY_TSTARTROBOT 20
#define PRIORITY_TCAMERA 21
#define PRIORITY_TSENDIMAGE 21
#define PRIORITY_TSEARCHARENA 20
#define PRIORITY_TCOMPUTEPOSITION 19

/*
 * Some remarks:
 * 1- This program is mostly a template. It shows you how to create tasks, semaphore
 *   message queues, mutex ... and how to use them
 * 
 * 2- semDumber is, as name say, useless. Its goal is only to show you how to use semaphore
 * 
 * 3- Data flow is probably not optimal
 * 
 * 4- Take into account that ComRobot::Write will block your task when serial buffer is full,
 *   time for internal buffer to flush
 * 
 * 5- Same behavior existe for ComMonitor::Write !
 * 
 * 6- When you want to write something in terminal, use cout and terminate with endl and flush
 * 
 * 7- Good luck !
 */

/**
 * @brief Initialisation des structures de l'application (tâches, mutex, 
 * semaphore, etc.)
 */
void Tasks::Init() {
    int status;
    int err;

    /**************************************************************************************/
    /* 	Mutex creation                                                                    */
    /**************************************************************************************/
    if (err = rt_mutex_create(&mutex_monitor, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_robot, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_robotStarted, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_move, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_cam, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_img, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_pos, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Mutexes created successfully" << endl << flush;

    /**************************************************************************************/
    /* 	Semaphors creation       							  */
    /**************************************************************************************/
    if (err = rt_sem_create(&sem_barrier, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_openComRobot, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_serverOk, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_startRobot, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_searchArena, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_arenaConfirm, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Semaphores created successfully" << endl << flush;

    /**************************************************************************************/
    /* Tasks creation                                                                     */
    /**************************************************************************************/
    if (err = rt_task_create(&th_server, "th_server", 0, PRIORITY_TSERVER, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_sendToMon, "th_sendToMon", 0, PRIORITY_TSENDTOMON, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_receiveFromMon, "th_receiveFromMon", 0, PRIORITY_TRECEIVEFROMMON, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_openComRobot, "th_openComRobot", 0, PRIORITY_TOPENCOMROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_startRobot, "th_startRobot", 0, PRIORITY_TSTARTROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_move, "th_move", 0, PRIORITY_TMOVE, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_sendImage, "th_sendImage", 0, PRIORITY_TSENDIMAGE, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
     if (err = rt_task_create(&th_searchArena, "th_searchArena", 0, PRIORITY_TSEARCHARENA, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_computePosition, "th_computePosition", 0, PRIORITY_TCOMPUTEPOSITION, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Tasks created successfully" << endl << flush;

    /**************************************************************************************/
    /* Message queues creation                                                            */
    /**************************************************************************************/
    if ((err = rt_queue_create(&q_messageToMon, "q_messageToMon", sizeof (Message*)*50, Q_UNLIMITED, Q_FIFO)) < 0) {
        cerr << "Error msg queue create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Queues created successfully" << endl << flush;

}

/**
 * @brief Démarrage des tâches
 */
void Tasks::Run() {
    rt_task_set_priority(NULL, T_LOPRIO);
    int err;

    if (err = rt_task_start(&th_server, (void(*)(void*)) & Tasks::ServerTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_sendToMon, (void(*)(void*)) & Tasks::SendToMonTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_receiveFromMon, (void(*)(void*)) & Tasks::ReceiveFromMonTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_openComRobot, (void(*)(void*)) & Tasks::OpenComRobot, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_startRobot, (void(*)(void*)) & Tasks::StartRobotTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_move, (void(*)(void*)) & Tasks::MoveTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
     if (err = rt_task_start(&th_sendImage, (void(*)(void*)) & Tasks::SendImage, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_searchArena, (void(*)(void*)) & Tasks::SearchArena, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
     if (err = rt_task_start(&th_computePosition, (void(*)(void*)) & Tasks::ComputePosition, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Tasks launched" << endl << flush;
    //////////////////////
    
}

/**
 * @brief Arrêt des tâches
 */
void Tasks::Stop() {
    monitor.Close();
    robot.Close();
}

/**
 */
void Tasks::Join() {
    cout << "Tasks synchronized" << endl << flush;
    rt_sem_broadcast(&sem_barrier);
    pause();
}

/**
 * @brief Thread handling server communication with the monitor.
 */
void Tasks::ServerTask(void *arg) {
    int status;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are started)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task server starts here                                                        */
    /**************************************************************************************/
    rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
    status = monitor.Open(SERVER_PORT);
    rt_mutex_release(&mutex_monitor);

    cout << "Open server on port " << (SERVER_PORT) << " (" << status << ")" << endl;

    if (status < 0) throw std::runtime_error {
        "Unable to start server on port " + std::to_string(SERVER_PORT)
    };
    monitor.AcceptClient(); // Wait the monitor client
    cout << "Rock'n'Roll baby, client accepted!" << endl << flush;
    rt_sem_broadcast(&sem_serverOk);
}

/**
 * @brief Thread sending data to monitor.
 */
void Tasks::SendToMonTask(void* arg) {
    Message *msg;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task sendToMon starts here                                                     */
    /**************************************************************************************/
    rt_sem_p(&sem_serverOk, TM_INFINITE);

    while (1) {
        cout << "wait msg to send" << endl << flush;
        msg = ReadInQueue(&q_messageToMon);
        cout << "Send msg to mon: " << msg->ToString() << endl << flush;
        rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
        monitor.Write(msg); // The message is deleted with the Write
        rt_mutex_release(&mutex_monitor);
    }
}

/**
 * @brief Thread receiving data from monitor.
 */
void Tasks::ReceiveFromMonTask(void *arg) {
    Message *msgRcv;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task receiveFromMon starts here                                                */
    /**************************************************************************************/
    rt_sem_p(&sem_serverOk, TM_INFINITE);
    cout << "Received message from monitor activated" << endl << flush;

    while (1) {
        msgRcv = monitor.Read();
        cout << "Rcv <= " << msgRcv->ToString() << endl << flush;

        if (msgRcv->CompareID(MESSAGE_MONITOR_LOST)) {
            delete(msgRcv);
            
            ///////////////////////////////////////////////////
            //////////////////////////////////////////////////
            
            /*robot.Close();
            robotStarted = 0;
            camera.Close();
            monitor.AcceptClient(); // Wait the monitor client
            cout << "Rock'n'Roll baby, client accepted!" << endl << flush;
            robotStarted = 1;
            
            camera.Open(); */
            
            //////////////////////////////////////////////////////
            
            exit(-1);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_COM_OPEN)) {
            rt_sem_v(&sem_openComRobot);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_START_WITHOUT_WD)) {
            rt_sem_v(&sem_startRobot);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_GO_FORWARD) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_BACKWARD) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_LEFT) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_RIGHT) ||
                msgRcv->CompareID(MESSAGE_ROBOT_STOP)) {

            rt_mutex_acquire(&mutex_move, TM_INFINITE);
            move = msgRcv->GetID();
            rt_mutex_release(&mutex_move);
        }
        else if (msgRcv->CompareID(MESSAGE_CAM_OPEN)) {
            rt_mutex_acquire(&mutex_cam,TM_INFINITE);
            if (!camera.Open()) {
                Message * msgErr;
                msgErr = new Message(MESSAGE_ANSWER_NACK);
                WriteInQueue(&q_messageToMon, msgErr);
            }
            rt_mutex_release(&mutex_cam);
        }
        else if (msgRcv->CompareID(MESSAGE_CAM_CLOSE)) {
            rt_mutex_acquire(&mutex_cam,TM_INFINITE);
                camera.Close();
                Message * msgErr;
                msgErr = new Message(MESSAGE_ANSWER_ACK);
                WriteInQueue(&q_messageToMon, msgErr);
                rt_mutex_release(&mutex_cam);
            }
        
        else if(msgRcv->CompareID(MESSAGE_CAM_ASK_ARENA)) {
            StopPeriodic = true;
            rt_sem_v(&sem_searchArena);
            
        }
        else if (msgRcv->CompareID(MESSAGE_CAM_ARENA_CONFIRM)) {
            ArenaConfirm = true;
            rt_sem_v(&sem_arenaConfirm);
            
        }
        else if (msgRcv->CompareID(MESSAGE_CAM_ARENA_INFIRM)) {
            ArenaConfirm = false;
            rt_sem_v(&sem_arenaConfirm);
            
        }
        else if(msgRcv->CompareID(MESSAGE_CAM_POSITION_COMPUTE_START)) {
            AskPosition=true;
        }
        else if(msgRcv->CompareID(MESSAGE_CAM_POSITION_COMPUTE_STOP)) {
            AskPosition=false;
        }
        delete(msgRcv); // mus be deleted manually, no consumer
    }
}

/**
 * @brief Thread opening communication with the robot.
 */
void Tasks::OpenComRobot(void *arg) {
    int status;
    int err;
    ////////////
    

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task openComRobot starts here                                                  */
    /**************************************************************************************/
    while (1) {
        rt_sem_p(&sem_openComRobot, TM_INFINITE);
        cout << "Open serial com (";
        rt_mutex_acquire(&mutex_robot, TM_INFINITE);
        status = robot.Open();
        rt_mutex_release(&mutex_robot);
        cout << status;
        cout << ")" << endl << flush;

        Message * msgSend;
        if (status < 0) {
            msgSend = new Message(MESSAGE_ANSWER_NACK);
        } else {
            msgSend = new Message(MESSAGE_ANSWER_ACK);
        }
        WriteInQueue(&q_messageToMon, msgSend); // msgSend will be deleted by sendToMon
    }
    
}

/**
 * @brief Thread starting the communication with the robot.
 */
void Tasks::StartRobotTask(void *arg) {
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task startRobot starts here                                                    */
    /**************************************************************************************/
    
    
    while (1) {
        
        Message * msgSend;
        rt_sem_p(&sem_startRobot, TM_INFINITE);
        cout << "Start robot without watchdog (";
        rt_mutex_acquire(&mutex_robot, TM_INFINITE);
        msgSend = robot.Write(robot.StartWithoutWD());
        CompteurATrois(msgSend);
        rt_mutex_release(&mutex_robot);
        cout << msgSend->GetID();
        cout << ")" << endl;

        cout << "Movement answer: " << msgSend->ToString() << endl << flush;
        WriteInQueue(&q_messageToMon, msgSend);  // msgSend will be deleted by sendToMon
        
        ///////
        if (msgSend->GetID() == MESSAGE_ANSWER_ACK) {
            rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
            robotStarted = 1;
            rt_mutex_release(&mutex_robotStarted);
        }
     
    }
}

/**
 * @brief Thread handling control of the robot.
 */
void Tasks::MoveTask(void *arg) {
    int rs;
    int cpMove;

    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/
    rt_task_set_periodic(NULL, TM_NOW, 100000000);

    while (1) {
        rt_task_wait_period(NULL);
        cout << "Periodic movement update";
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        rs = robotStarted;
        rt_mutex_release(&mutex_robotStarted);
        if (rs == 1) {
            rt_mutex_acquire(&mutex_move, TM_INFINITE);
            cpMove = move;
            rt_mutex_release(&mutex_move);
            
            cout << " move: " << cpMove;
            
            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            CompteurATrois(robot.Write(new Message((MessageID)cpMove)));
           
            rt_mutex_release(&mutex_robot);
        }
        cout << endl << flush;
        
    }
}

//////

void Tasks::SendImage(void *arg) {
            
    rt_task_set_periodic(NULL, TM_NOW, 100000000);
    while (1) {
        rt_task_wait_period(NULL);  
        rt_mutex_acquire(&mutex_cam, TM_INFINITE);
        if(camera.IsOpen() and !StopPeriodic) {
            //Img * img = new Img(camera.Grab());
            rt_mutex_acquire(&mutex_img, TM_INFINITE);
            img = new Img(camera.Grab());
            cout<<"je capture l'image"<<endl<<flush;
            if (!arena.IsEmpty()) {
                img->DrawArena(arena);
                
            }
            rt_mutex_acquire(&mutex_pos, TM_INFINITE);
            if(!lposition.empty()) {
                img->DrawRobot(lposition.front());
            }
            rt_mutex_release(&mutex_pos);
            
              MessageImg *msgImg = new MessageImg(MESSAGE_CAM_IMAGE, img); 
              rt_mutex_release(&mutex_img);
              rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
              monitor.Write(msgImg); // The message is deleted with the Write
              rt_mutex_release(&mutex_monitor);
            
            
          
        }
        rt_mutex_release(&mutex_cam);
    }
    
}

/**
 * Write a message in a given queue
 * @param queue Queue identifier
 * @param msg Message to be stored
 */
void Tasks::WriteInQueue(RT_QUEUE *queue, Message *msg) {
    int err;
    if ((err = rt_queue_write(queue, (const void *) &msg, sizeof ((const void *) &msg), Q_NORMAL)) < 0) {
        cerr << "Write in queue failed: " << strerror(-err) << endl << flush;
        throw std::runtime_error{"Error in write in queue"};
    }
}

/**
 * Read a message from a given queue, block if empty
 * @param queue Queue identifier
 * @return Message read
 */
Message *Tasks::ReadInQueue(RT_QUEUE *queue) {
    int err;
    Message *msg;

    if ((err = rt_queue_read(queue, &msg, sizeof ((void*) &msg), TM_INFINITE)) < 0) {
        cout << "Read in queue failed: " << strerror(-err) << endl << flush;
        throw std::runtime_error{"Error in read in queue"};
    }/** else {CLOSE)
        cout << "@msg :" << msg << endl << flush;
    } /**/

    return msg;
}

void Tasks::CompteurATrois(Message * msgSend) {
    if ((msgSend->GetID()==MESSAGE_ANSWER_NACK) ||(msgSend->GetID()==MESSAGE_ANSWER_ROBOT_ERROR) || (msgSend->GetID()==MESSAGE_ANSWER_COM_ERROR )|| (msgSend->GetID()==MESSAGE_ANSWER_ROBOT_TIMEOUT) || (msgSend->GetID()==MESSAGE_ANSWER_ROBOT_UNKNOWN_COMMAND)) {
        cmpt++;
        //cout << "Le compteur est à " <<cmpt<< endl << flush;
    }
    else {
        cmpt = 0;
        //cout<<"Je suis dans la fonction CompteurATrois"<<endl<<flush;
    }
    if (cmpt > 3)
    {
        Message * msgErr;
        msgErr = new Message(MESSAGE_ANSWER_COM_ERROR);
        WriteInQueue(&q_messageToMon, msgErr);
        rt_mutex_acquire(&mutex_move, TM_INFINITE);
        move = MESSAGE_ROBOT_STOP;
        rt_mutex_release(&mutex_move);
        robot.Close();
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        robotStarted = 0;
        rt_mutex_release(&mutex_robotStarted);
        rt_sem_v(&sem_openComRobot);
    }
    
}

void Tasks::SearchArena(void *arg) {
    
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    while (1) {
        
    rt_sem_p(&sem_searchArena, TM_INFINITE);
    
    Message *msgRcv;
    //rt_mutex_acquire(&mutex_cam, TM_INFINITE);
    //Img * img = new Img(camera.Grab());
    //img = new Img(camera.Grab());
    //rt_mutex_release(&mutex_cam);
    Arena arenaTest;
    //Arena * arena = new Arena;
    rt_mutex_acquire(&mutex_img, TM_INFINITE);
    arenaTest = img->SearchArena();
     rt_mutex_release(&mutex_img);
    
    if (arenaTest.IsEmpty()) {
        Message * msgErr;
        msgErr = new Message(MESSAGE_ANSWER_NACK);
        WriteInQueue(&q_messageToMon, msgErr);
    }
    else {
        cout<<"Je suis dans la confirmation"<<endl<<flush;
         rt_mutex_acquire(&mutex_img, TM_INFINITE);
        img->DrawArena(arenaTest);
        MessageImg *msgImg = new MessageImg(MESSAGE_CAM_IMAGE, img);
         rt_mutex_release(&mutex_img);
        rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
        monitor.Write(msgImg); // The message is deleted with the Write
        rt_mutex_release(&mutex_monitor);
        rt_sem_p(&sem_arenaConfirm, TM_INFINITE);
        cout<<"Semaphore libéré"<<endl<<flush;
        if (ArenaConfirm) {
            arena=arenaTest;
            cout<<"L'arène est verifiée reprise de l'envoi"<<endl<<flush;
        }
        cout<<"Je suis à la fin"<<endl<<flush;
        
        
    }
    StopPeriodic = false;
    cout<<"StopPeriodic=false"<<endl<<flush;
    //return arena;
    
}
}

void Tasks::ComputePosition(void *arg) {
            
    rt_task_set_periodic(NULL, TM_NOW, 100000000);
    
    while (1) {
        rt_task_wait_period(NULL);
        if (AskPosition) {
        Message *msg;
        MessagePosition * msgPos;
        //rt_mutex_acquire(&mutex_cam, TM_INFINITE);
        //Img * img = new Img(camera.Grab());
        //rt_mutex_release(&mutex_cam);
        rt_mutex_acquire(&mutex_img, TM_INFINITE);
        rt_mutex_acquire(&mutex_pos, TM_INFINITE);
        lposition = img->SearchRobot(arena);
        cout<<"je calcule la position"<<endl<<flush;
        rt_mutex_release(&mutex_img);
        if(!lposition.empty()) {
            
            msgPos = new MessagePosition(MESSAGE_CAM_POSITION, lposition.front());
            rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
            monitor.Write(msgPos); // The message is deleted with the Write
            rt_mutex_release(&mutex_monitor);
        }
        else {
            cout<<"Position nulle"<<endl<<flush;
        }
        rt_mutex_release(&mutex_pos);
        rt_mutex_acquire(&mutex_img, TM_INFINITE);
        MessageImg *msgImg = new MessageImg(MESSAGE_CAM_IMAGE, img);
         rt_mutex_release(&mutex_img);
        rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
        monitor.Write(msgImg); // The message is deleted with the Write
        rt_mutex_release(&mutex_monitor);
    }
    }
}