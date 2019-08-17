#include "mpga_emergent_behavior_loop_functions.h"
#include <buzz/argos/buzz_controller.h>


/****************************************/
/****************************************/

CMPGAEmergentBehaviorLoopFunctions::CMPGAEmergentBehaviorLoopFunctions() {
    /*
     * Create the random number generator
     */
    m_pcRNG = CRandom::CreateRNG("argos");
    m_pfControllerParams.resize(GENOME_SIZE);
}

/****************************************/
/****************************************/

CMPGAEmergentBehaviorLoopFunctions::~CMPGAEmergentBehaviorLoopFunctions() {
    m_pfControllerParams.clear();
}

/****************************************/
/****************************************/

/**
 * Functor to put the stimulus in the Buzz VMs.
 */
struct PutGenome : public CBuzzLoopFunctions::COperation {

    /** Constructor */
    explicit PutGenome(const std::vector<double> &vec_genome) : m_vecGenome(vec_genome) {}

    /** The action happens here */
    virtual void operator()(const std::string &str_robot_id,
                            buzzvm_t t_vm) {
        /* Set the values of the table 'genome' in the Buzz VM */
        BuzzTableOpen(t_vm, "genome");
        for(int i = 0; i < m_vecGenome.size(); ++i) {
            BuzzTablePut(t_vm, i, (float) m_vecGenome[i]);
        }
        BuzzTableClose(t_vm);
    }

    /** Genome */
    const std::vector<double> &m_vecGenome;
};


/****************************************/
/****************************************/

/**
 * Functor to get data from the robots
 */
struct GetRobotData : public CBuzzLoopFunctions::COperation {

    /** Constructor */
    //temp is unneeded, but if we get rid of it, it errors out. So we left it in *shrug*
    GetRobotData(int t) : temp(t) {}

    /** The action happens here */
    virtual void operator()(const std::string &str_robot_id,
                            buzzvm_t t_vm) {

        // get the buzzobj corresponding to the value we want
        buzzobj_t tCurX = BuzzGet(t_vm, "cur_x");

        // confirm the value is the type we expect. Print error and then exit if they don't match
        if (!buzzobj_isfloat(tCurX)) {
            LOGERR << str_robot_id << ": variable 'cur_x' has wrong type " << buzztype_desc[tCurX->o.type] << std::endl;
            return;
        }

        //Cast the value to the appropriate type and variable
        float fCurX = buzzobj_getfloat(tCurX);

        //Repeat the process with every other variable
        buzzobj_t tCurY = BuzzGet(t_vm, "cur_y");
        if (!buzzobj_isfloat(tCurY)) {
            LOGERR << str_robot_id << ": variable 'cur_y' has wrong type " << buzztype_desc[tCurY->o.type] << std::endl;
            return;
        }

        float fCurY = buzzobj_getfloat(tCurY);

        buzzobj_t tCurZ = BuzzGet(t_vm, "cur_z");
        if (!buzzobj_isfloat(tCurZ)) {
            LOGERR << str_robot_id << ": variable 'cur_z' has wrong type " << buzztype_desc[tCurZ->o.type] << std::endl;
            return;
        }

        float fCurZ = buzzobj_getfloat(tCurZ);

        buzzobj_t tCurS = BuzzGet(t_vm, "cur_s");
        if (!buzzobj_isint(tCurS)) {
            LOGERR << str_robot_id << ": variable 'cur_s' has wrong type " << buzztype_desc[tCurS->o.type] << std::endl;
            return;
        }

        int iCurS = buzzobj_getint(tCurS);


        buzzobj_t tAvg0 = BuzzGet(t_vm, "avg_time_0");
        if (!buzzobj_isfloat(tAvg0)) {
            LOGERR << str_robot_id << ": variable 'avg_time_0' has wrong type " << buzztype_desc[tAvg0->o.type] << std::endl;
            return;
        }

        float fAvg0 = buzzobj_getfloat(tAvg0);

        buzzobj_t tAvg1 = BuzzGet(t_vm, "avg_time_1");
        if (!buzzobj_isfloat(tAvg1)) {
            LOGERR << str_robot_id << ": variable 'avg_time_1' has wrong type " << buzztype_desc[tAvg1->o.type] << std::endl;
            return;
        }

        float fAvg1 = buzzobj_getfloat(tAvg1);



        buzzobj_t tCurR = BuzzGet(t_vm, "cur_r");
        if (!buzzobj_isint(tCurR)) {
            LOGERR << str_robot_id << ": variable 'cur_r' has wrong type " << buzztype_desc[tCurR->o.type] << std::endl;
            return;
        }

        int iCurR = buzzobj_getint(tCurR);

        buzzobj_t tCurSpeed = BuzzGet(t_vm, "cur_speed");
        if (!buzzobj_isfloat(tCurSpeed)) {
            LOGERR << str_robot_id << ": variable 'cur_speed' has wrong type " << buzztype_desc[tCurSpeed->o.type] << std::endl;
            return;
        }

        float fCurSpeed = buzzobj_getfloat(tCurSpeed);

        //Add each value to the back of the vector that stores all this
        m_vecRobotX.push_back(fCurX);
        m_vecRobotY.push_back(fCurY);
        m_vecRobotZ.push_back(fCurZ);
        m_vecRobotSpeed.push_back(fCurSpeed);
        m_vecRobotReading.push_back(iCurR);
        m_vecRobotState.push_back(iCurS);
        m_vecAvgRobotState0.push_back(fAvg0);
        m_vecAvgRobotState1.push_back(fAvg1);

    }

    int temp;

    std::vector<float> m_vecRobotX;
    std::vector<float> m_vecRobotY;
    std::vector<float> m_vecRobotZ;
    std::vector<int> m_vecRobotState;
    std::vector<int> m_vecRobotReading;
    std::vector<float> m_vecRobotSpeed;
    std::vector<float> m_vecAvgRobotState0;
    std::vector<float> m_vecAvgRobotState1;

};

/****************************************/
/****************************************/


void CMPGAEmergentBehaviorLoopFunctions::Init(TConfigurationNode &t_node) {

    // printErr("Statrted Init");
    //From the argos XML, get the number of robots we want to test with
    UInt32 iNumRobots;
    GetNodeAttribute(t_node, "num_robots", iNumRobots);

    //Create, locate, and place all the robots
    CreateRobots(iNumRobots);
    // printErr("Got numRobots");

    /*
     * Process trial information, if any
     * Not really used for us, unless you wish to trial a genome
     */
    try {
        UInt32 unTrial;
        GetNodeAttribute(t_node, "trial", unTrial);
        SetTrial(unTrial);
        Reset();
    } catch (CARGoSException &ex) {}
    // printErr("Fiished INIT");
}

/****************************************/
/****************************************/

void CMPGAEmergentBehaviorLoopFunctions::Reset() {
    /*
     * Move robot to the initial position corresponding to the current trial
     */
    // printErr("Started Reset");

    //For each robot, check to see if it moved, if it had, put it back. IF it errors out, print the robot and where we tried to move it to
    for(size_t i = 0; i < m_vecKheperas.size(); i++) {
        if(!MoveEntity(
                    m_vecKheperas[i]->GetEmbodiedEntity(),        //Move this robot
                    m_vecInitSetup[i].Position,          // with this position
                    m_vecInitSetup[i].Orientation,       // with this orientation
                    false                                         // this is not a check, so actually move the robot back
                )) {
            LOGERR << "Can't move robot kh(" << i << ") in <"
                   << m_vecInitSetup[i].Position
                   << ">, <"
                   << m_vecInitSetup[i].Orientation
                   << ">"
                   << std::endl;
        }
    }
    // printErr("Finished Reset");
}

/****************************************/
/****************************************/
//For whatever reason, if you don't put the genome back each time step, it defaults back to the 0.0 defaults.
void CMPGAEmergentBehaviorLoopFunctions::PreStep() {
    PutGenome cPutGenome(m_pfControllerParams);
    BuzzForeachVM(cPutGenome);
}
/****************************************/
/****************************************/

//This is just a wrapper that takes the genome values supplied by MPGA and places them in the buzz script
void CMPGAEmergentBehaviorLoopFunctions::ConfigureFromGenome(const Real *pf_genome) {
    printErr("Started Config from Genome");
    /* Copy the genes into the NN parameter buffer */
    for (size_t i = 0; i < GENOME_SIZE; ++i) {
        m_pfControllerParams[i] = pf_genome[i];
    }
    PutGenome cPutGenome(m_pfControllerParams);
    BuzzForeachVM(cPutGenome);
    // printErr("finished config from genome");
}

/****************************************/
/****************************************/

CAnalysis::AnalysisResults
CMPGAEmergentBehaviorLoopFunctions::AnalyzeSwarm(int swarmID, const std::vector<float> &vecRobotX,
        const std::vector<float> &vecRobotY,
        const std::vector<float> &vecRobotZ,
        const std::vector<float> &vecRobotSpeed,
        const std::vector<int> &vecRobotState,
        const std::vector<float> &vecAvgRobotState0,
        const std::vector<float> &vecAvgRobotState1) {
    std::vector<int> stateIndexes;
    for (int i = 0; i < vecRobotState.size(); ++i) {
        if (vecRobotState[i] == swarmID) {
            stateIndexes.push_back(i);
        }
    }

    std::vector<float> filteredRobotX;
    std::vector<float> filteredRobotY;
    std::vector<float> filteredRobotZ;
    std::vector<float> filteredRobotSpeed;
    std::vector<int> filteredRobotState;
    std::vector<float> filteredRobotState0;
    std::vector<float> filteredRobotState1;

    for(auto index : stateIndexes) {
        filteredRobotX.push_back(vecRobotX[index]);
        filteredRobotY.push_back(vecRobotY[index]);
        filteredRobotZ.push_back(vecRobotZ[index]);
        filteredRobotSpeed.push_back(vecRobotSpeed[index]);
        filteredRobotState.push_back(vecRobotState[index]);
        filteredRobotState0.push_back(vecAvgRobotState0[index]);
        filteredRobotState1.push_back(vecAvgRobotState1[index]);
    }

    CAnalysis analysis(filteredRobotX, filteredRobotY, filteredRobotZ, filteredRobotSpeed, filteredRobotState, filteredRobotState0, filteredRobotState1);
    return analysis.AnalyzeAll();
}

/****************************************/
/****************************************/

//Oh buddy
Real CMPGAEmergentBehaviorLoopFunctions::Score() {
    // printErr("Started Score");
    //Get the robot data
    GetRobotData cGetRobotData(0);
    BuzzForeachVM(cGetRobotData);
    // printErr("Finished Get data");

    //Create the Analysis class, with the vectors from the data
    CAnalysis fullSwarmAnalysis(cGetRobotData.m_vecRobotX, cGetRobotData.m_vecRobotY, cGetRobotData.m_vecRobotZ,
                                cGetRobotData.m_vecRobotSpeed, cGetRobotData.m_vecRobotState, cGetRobotData.m_vecAvgRobotState0, cGetRobotData.m_vecAvgRobotState1);

    //Analyze all, and save the results in a struct that has each value.
    CAnalysis::AnalysisResults fullSwarmResults = fullSwarmAnalysis.AnalyzeAll();

    CAnalysis::AnalysisResults swarm0Results = AnalyzeSwarm(0, cGetRobotData.m_vecRobotX, cGetRobotData.m_vecRobotY, cGetRobotData.m_vecRobotZ,
            cGetRobotData.m_vecRobotSpeed, cGetRobotData.m_vecRobotState, cGetRobotData.m_vecAvgRobotState0, cGetRobotData.m_vecAvgRobotState1);

    CAnalysis::AnalysisResults swarm1Results = AnalyzeSwarm(1, cGetRobotData.m_vecRobotX, cGetRobotData.m_vecRobotY, cGetRobotData.m_vecRobotZ,
            cGetRobotData.m_vecRobotSpeed, cGetRobotData.m_vecRobotState, cGetRobotData.m_vecAvgRobotState0, cGetRobotData.m_vecAvgRobotState1);
    //Open the master file as READ-ONLY. This is important, you can open a file from multiple locations read only, but if you try to open it to write things get fucky. Don't let them get fucky.
    std::ifstream score_files("master_scores.csv", std::ios::in);

    //Variables for reference later
    std::string line;
    Real minDistance = 99999;
    // printErr("Started Scoring");

    //This makes sure we ignore the first value, which are the names of the csv columns
    bool skipped = false;

    //Confirm the file opened
    if(score_files.is_open()) {
        //While loop to iterate over each line. getLine will return 0 when there are no more lines, exiting the loop
        while(getline( score_files, line)) {
            if(skipped) {
                //Make and array the size of the genome + the size of the feature values for the whole swarm and each state + 1 for the score tacked onto the end
                double scores[GENOME_SIZE + fullSwarmResults.size + swarm0Results.size + swarm1Results.size + 1 - 2 - 2]; //Genomes, feature scores, +1 for the actual score that gets added

                //Parse the line and store the values in scores array
                ParseValues(line, (UInt32) GENOME_SIZE + 20, scores, ',');

                //Pull out the values for easier reference
                Real comp_full_scatter = scores[GENOME_SIZE];
                Real comp_full_variance = scores[GENOME_SIZE + 1];
                Real comp_full_speed = scores[GENOME_SIZE + 2];
                Real comp_full_angMomentum = scores[GENOME_SIZE + 3];
                Real comp_full_groupRotation = scores[GENOME_SIZE + 4];
                Real comp_full_state0 = scores[GENOME_SIZE + 5];
                Real comp_full_avg0 = scores[GENOME_SIZE + 6];
                Real comp_full_avg1 = scores[GENOME_SIZE + 7];

                Real comp_swarm0_scatter = scores[GENOME_SIZE + 8];
                Real comp_swarm0_variance = scores[GENOME_SIZE + 9];
                Real comp_swarm0_speed = scores[GENOME_SIZE + 10];
                Real comp_swarm0_angMomentum = scores[GENOME_SIZE + 11];
                Real comp_swarm0_groupRotation = scores[GENOME_SIZE + 12];
                Real comp_swarm0_state0 = scores[GENOME_SIZE + 13];


                Real comp_swarm1_scatter = scores[GENOME_SIZE + 14];
                Real comp_swarm1_variance = scores[GENOME_SIZE + 15];
                Real comp_swarm1_speed = scores[GENOME_SIZE + 16];
                Real comp_swarm1_angMomentum = scores[GENOME_SIZE + 17];
                Real comp_swarm1_groupRotation = scores[GENOME_SIZE + 18];
                Real comp_swarm1_state0 = scores[GENOME_SIZE + 19];


                //State difference is not calculated as part of the score
                //Same with the previous generations

                //Find the distance from basic Pythagorean method. Dist is the score
                Real dist = sqrt(
                                pow(comp_full_scatter - fullSwarmResults.Scatter, 2) +
                                pow(comp_full_variance - fullSwarmResults.RadialVariance, 2) +
                                pow(comp_full_speed - fullSwarmResults.Speed, 2) +
                                pow(comp_full_angMomentum - fullSwarmResults.AngularMomentum, 2) +
                                pow(comp_full_groupRotation - fullSwarmResults.GroupRotation, 2) +
                                pow(comp_full_state0 - fullSwarmResults.StateZeroCount, 2) +
                                pow(comp_full_avg0 - fullSwarmResults.AvgStateTime0, 2) +
                                pow(comp_full_avg1 - fullSwarmResults.AvgStateTime1, 2) +

                                pow(comp_swarm0_scatter - swarm0Results.Scatter, 2) +
                                pow(comp_swarm0_variance - swarm0Results.RadialVariance, 2) +
                                pow(comp_swarm0_speed - swarm0Results.Speed, 2) +
                                pow(comp_swarm0_angMomentum - swarm0Results.AngularMomentum, 2) +
                                pow(comp_swarm0_groupRotation - swarm0Results.GroupRotation, 2) +
                                pow(comp_swarm0_state0 - swarm0Results.StateZeroCount, 2) +


                                pow(comp_swarm1_scatter - swarm1Results.Scatter, 2) +
                                pow(comp_swarm1_variance - swarm1Results.RadialVariance, 2) +
                                pow(comp_swarm1_speed - swarm1Results.Speed, 2) +
                                pow(comp_swarm1_angMomentum - swarm1Results.AngularMomentum, 2) +
                                pow(comp_swarm1_groupRotation - swarm1Results.GroupRotation, 2) +
                                pow(comp_swarm1_state0 - swarm1Results.StateZeroCount, 2)
                            );

                //We want to find the shortest distance
                if (dist < minDistance) {
                    minDistance = dist;
                }
            } else {
                skipped = true;
            }
        }
    } else {
        minDistance = 0;
    }
    // printErr("Finished Scoring");

    //Close the master score file. VERY IMPORTANT
    score_files.close();


    // printErr("Flushing genomes to inidividual files");

    //Open the file for that genome.
    std::ofstream cScoreFile(std::string("score_" + ToString(::getpid()) + ".csv").c_str(), std::ios::out | std::ios::trunc);

    //Confirm the file is open
    if(cScoreFile.is_open()) {
        //Output each genome value
        for(auto val : m_pfControllerParams) {
            cScoreFile << val << ',';
        }

        //Output the feature values and the score (minDistance)
        //TODO ADD ALL THE EXTRA READINGS
        cScoreFile
                << fullSwarmResults.Scatter << ','
                << fullSwarmResults.RadialVariance << ','
                << fullSwarmResults.Speed << ','
                << fullSwarmResults.AngularMomentum << ','
                << fullSwarmResults.GroupRotation << ','
                << fullSwarmResults.StateZeroCount << ','
                << fullSwarmResults.AvgStateTime0 << ','
                << fullSwarmResults.AvgStateTime1 << ','

                << swarm0Results.Scatter << ','
                << swarm0Results.RadialVariance << ','
                << swarm0Results.Speed << ','
                << swarm0Results.AngularMomentum << ','
                << swarm0Results.GroupRotation << ','
                << swarm0Results.StateZeroCount << ','


                << swarm1Results.Scatter << ','
                << swarm1Results.RadialVariance << ','
                << swarm1Results.Speed << ','
                << swarm1Results.AngularMomentum << ','
                << swarm1Results.GroupRotation << ','
                << swarm1Results.StateZeroCount << ','


                << minDistance << std::endl;
    } else {
        //panic
    }
    //Close the score file
    cScoreFile.close();

    /* The performance is simply the distance of the robot to the origin */
    return minDistance;
}

/****************************************/
/****************************************/
void CMPGAEmergentBehaviorLoopFunctions::CreateRobots(UInt32 un_robots) {
    //The angular gap between each robot. Dependent on the number of robots
    CRadians robStep = CRadians::TWO_PI / static_cast<Real>(un_robots);
    CRadians cOrient;

    //for each robot, calculate the positon based on spherical coordinates
    for(size_t i = 0; i < un_robots; ++i) {
        CVector3 pos;
        pos.FromSphericalCoords(
            2.0f,
            CRadians::PI_OVER_TWO,
            CRadians(i * robStep));
        //Make sure they're on the ground, otherwise it breaks
        pos.SetZ(0.0);

        //Randomly choose the starting orientation
        CQuaternion head;
        cOrient = m_pcRNG->Uniform(CRadians::UNSIGNED_RANGE);
        head.FromEulerAngles(
            cOrient,        // rotation around Z
            CRadians::ZERO, // rotation around Y
            CRadians::ZERO  // rotation around X
        );
        /* Create robot */
        CKheperaIVEntity *pcRobot = new CKheperaIVEntity(
            "kh" + ToString(i),
            KH_CONTROLLER,
            pos,
            head,
            KH_COMMRANGE,
            KH_DATASIZE);
        /* Add it to the simulation */
        AddEntity(*pcRobot);
        /* Add it to the internal lists */
        m_vecKheperas.push_back(pcRobot);
        m_vecControllers.push_back(
            &dynamic_cast<CBuzzController &>(
                pcRobot->GetControllableEntity().GetController()));

        SInitSetup str;
        str.Position = pos;
        str.Orientation = head;
        m_vecInitSetup.push_back(str);
    }
    BuzzRegisterVMs();
}

//Easy wrapper for printing to the error log so when you get a "Blah failed. Check ARGoS_LOGERR_#### there's actually something useful there"
void CMPGAEmergentBehaviorLoopFunctions::printErr(std::string in) {
    LOGERR << in << std::endl;
    LOGERR.Flush();
}


//Register the loop functions so buzz and ARGoS can find it
REGISTER_LOOP_FUNCTIONS(CMPGAEmergentBehaviorLoopFunctions, "mpga_emergent_behavior_loop_functions")