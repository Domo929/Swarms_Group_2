/*
 * This is a simple example of a multi-process genetic algorithm that
 * uses multiple processes to parallelize the optimization process.
 */

#include <iostream>
#include <fstream>
#include <loop_functions/mpga.h>
#include <loop_functions/mpga_emergent_behavior_loop_functions.h>

/*
 * Flush best individual
 * Writes gnome to file "best"
 */
void FlushIndividual(const CMPGA::SIndividual &s_ind,
                     UInt32 un_generation) {
    std::ostringstream cOSS;
    cOSS << "best_" << un_generation << ".dat";
    std::ofstream cOFS(cOSS.str().c_str(), std::ios::out | std::ios::trunc);
    /* First write the number of values to dump */
    cOFS << GENOME_SIZE;
    /* Then dump the genome */
    for (UInt32 i = 0; i < GENOME_SIZE; ++i) {
        cOFS << " " << s_ind.Genome[i];
    }
    /* End line */
    cOFS << std::endl;
}

/*
* add all scores from each generation from the master file
*/
void FlushToMasterFile(const std::vector<pid_t> &slavePIDs, UInt32 randSeed, UInt32 genNumber) {
    std::ofstream masterScoreFile("master_scores.csv", std::ios::out | std::ios::app);
    if (masterScoreFile.is_open()) {
        for (auto pid : slavePIDs) {
            std::ifstream indScoreFile(std::string("score_" + ToString(pid) + ".csv").c_str(), std::ios::in);
            std::string line;
            if (indScoreFile.is_open()) {
                while (getline(indScoreFile, line)) {
                    masterScoreFile << line << ","
                                    << ToString(randSeed) + "_" + ToString(pid) + "_" + ToString(genNumber)
                                    << std::endl;
                }
            }
            indScoreFile.close();
        }
    }
    masterScoreFile.close();
}

void RenameExperiments(const std::vector<pid_t> &slavePIDs, UInt32 randSeed, UInt32 genNumber) {
    for (auto pid : slavePIDs) {
        std::string old_name = "experiment_" + ToString(pid) + ".csv";
        std::string new_name =
                "experiment_" + ToString(randSeed) + "_" + ToString(pid) + "_" + ToString(genNumber) + ".csv";
        std::rename(old_name.c_str(), new_name.c_str());
    }
}
/*
* called when mater file gets created
* names all columns in the csv
*/
void FlushNamesToMasterFile() {
    std::ofstream masterScoreFile("master_scores.csv", std::ios::out | std::ios::app);
    if (masterScoreFile.is_open()) {
        masterScoreFile <<
                        "s'00" << "," <<
                        "vl00" << "," <<
                        "vr00" << "," <<
                        "s'01" << "," <<
                        "vl01" << "," <<
                        "vr01" << "," <<
                        "s'10" << "," <<
                        "vl10" << "," <<
                        "vr10" << "," <<
                        "s'11" << "," <<
                        "vl11" << "," <<
                        "vr11" << "," <<
                        "s'20" << "," <<
                        "vl20" << "," <<
                        "vr20" << "," <<
                        "s'21" << "," <<
                        "vl21" << "," <<
                        "vr21" << "," <<
                        "full_cent_x" << "," <<
                        "full_cent_y" << "," <<
                        "full_scatter" << "," <<
                        "full_radialVar" << "," <<
                        "full_speed" << "," <<
                        "full_angMom" << "," <<
                        "full_grpRot" << "," <<
                        "full_state_change_freq" << "," <<
                        "0_cent_x" << "," <<
                        "0_cent_y" << "," <<
                        "0_scatter" << "," <<
                        "0_radialVar" << "," <<
                        "0_speed" << "," <<
                        "0_angMom" << "," <<
                        "0_grpRot" << "," <<
                        "1_cent_x" << "," <<
                        "1_cent_y" << "," <<
                        "1_scatter" << "," <<
                        "1_radialVar" << "," <<
                        "1_speed" << "," <<
                        "1_angMom" << "," <<
                        "1_grpRot" << "," <<
                        "Score" << "," <<
                        "ExpID" << std::endl;

    }
    masterScoreFile.close();
}


/*
 * The function used to aggregate the scores of each trial.  In this
 * experiment, the score is the distance of the robot from the
 * light. We take the maximum value as aggregated score.
 */
Real ScoreAggregator(const std::vector<Real> &vec_scores) {
    Real fScore = vec_scores[0];
    return fScore;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Didn't provide the randseed" << std::endl;
        return 1;
    }

    auto randSeed = (UInt32) atoi(argv[1]);

    CMPGA cGA(CRange<Real>(0, 10.0),                    // Allele range
              GENOME_SIZE,                              // Genome size
              8, //change this to number of cores       // Population size
              0.1,                                     // Mutation probability
              1,                                        // Number of trials
              1000, //make this not two                    // Number of generations
              true,                                     // Maximize score (False will minimize score)
              "/home/djcupo/Emergent-Behavior-From-Changeable-Internal-State/experiments/emergent_behavior.argos",    // .argos conf file
              &ScoreAggregator,                         // The score aggregator
              randSeed                                  // Random seed
    );
    FlushNamesToMasterFile();
    cGA.Evaluate();
    argos::LOG << "Generation #" << cGA.GetGeneration() << "...";
    argos::LOG << " scores:";
    for (auto pop : cGA.GetPopulation()) {
        argos::LOG << " " << pop->Score;
    }
    LOG << std::endl;
    LOG.Flush();
    UInt32 counter = 0;

    while (!cGA.Done()) {
        cGA.NextGen();
        cGA.Evaluate();
        argos::LOG << "Generation #" << cGA.GetGeneration() << "...";
        argos::LOG << " scores:";
        for (UInt32 i = 0; i < cGA.GetPopulation().size(); ++i) {
            argos::LOG << " " << cGA.GetPopulation()[i]->Score;
        }
//        if (cGA.GetGeneration() % 5 == 0) {
//            argos::LOG << " [Flushing genome... ";
//            /* Flush scores of best individual */
//            FlushIndividual(*cGA.GetPopulation()[0],
//                            cGA.GetGeneration());
//            argos::LOG << "done.]";
//        }
        LOG << std::endl;
        LOG.Flush();

        FlushToMasterFile(cGA.getSlavePIDs(), randSeed, cGA.m_unCurrentGeneration);
        RenameExperiments(cGA.getSlavePIDs(), randSeed, counter);
        counter++;

        LOG.Flush();
    }
    return 0;
}
