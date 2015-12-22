#include <vector>
#include <iostream>
#include "HubAlignWrapper.hpp"
#include "../measures/localMeasures/Sequence.hpp"

using namespace std;

const string HubAlignWrapper::hubalignProgram = "./HubAlign/HubAlign";

HubAlignWrapper::HubAlignWrapper(Graph* G1, Graph* G2, double alpha): Method(G1, G2, "HubAlign"),
    alpha(alpha), g1Name(G1->getName()), g2Name(G2->getName()) {

    g1Folder = "networks/" + g1Name + "/";
    g2Folder = "networks/" + g2Name + "/";

    g1EdgeListFile = g1Folder + "autogenerated/" + g1Name + "_edgeList.txt";
    g2EdgeListFile = g2Folder + "autogenerated/" + g2Name + "_edgeList.txt";

    similarityFile = "sequence/bitscores/" + g1Name + "_" + g2Name + ".bitscores";

    //rand int used to avoid collision between parallel runs
    g1TmpFile = "hubaligntmp1_"+g1Name+"_"+g2Name+"_"+intToString(randInt(0, 999999));
    g2TmpFile = "hubaligntmp2_"+g1Name+"_"+g2Name+"_"+intToString(randInt(0, 999999));

    alignmetFile = g1TmpFile + "-" + g2TmpFile + ".alignment";
}

void HubAlignWrapper::generateEdgeListFile(int graphNum) {
    Graph* G;
    if (graphNum == 1) G = G1;
    else G = G2;

    vector<vector<ushort> > edgeList;
    G->getEdgeList(edgeList);
    map<ushort,string> names = G->getIndexToNodeNameMap();
    uint m = G->getNumEdges();
    vector<vector<string> > edgeListNames(m, vector<string> (2));
    for (uint i = 0; i < m; i++) {
        edgeListNames[i][0] = names[edgeList[i][0]];
        edgeListNames[i][1] = names[edgeList[i][1]];
    }

    string file = graphNum == 1 ? g1EdgeListFile : g2EdgeListFile;
    writeDataToFile(edgeListNames, file, true);
}

void HubAlignWrapper::copyEdgeListsToTmpFiles() {
    exec("cp " + g1EdgeListFile + " " + g1TmpFile);
    exec("cp " + g2EdgeListFile + " " + g2TmpFile);
}

//Examples of executions of HubAlign (if alpha==1 sim file is not needed)
//./HubAlign Test1.tab Test2.tab –l 0.1 –a 0.7 –d 10 –b similarityFile.txt
//./HubAlign Test1.tab Test2.tab –l 0.1 –a 1 
void HubAlignWrapper::generateAlignment() {
    //in hubalign alpha is the fraction of topology
    double reversedAlpha = 1 - alpha;
    
    exec("chmod +x "+hubalignProgram);
    string cmd = hubalignProgram + " " + g1TmpFile + " " + g2TmpFile;
    cmd += " -l 0.1 -a " + to_string(reversedAlpha);
    cmd += " -d 10 ";
    if (reversedAlpha < 1) {
        cmd += " -b " + similarityFile;
    }
    cerr << "Executing " << cmd << endl;
    execPrintOutput(cmd);
    cerr << "Done" << endl;
}

void HubAlignWrapper::deleteAuxFiles() {
    string evalFile = g1TmpFile + "-" + g2TmpFile + ".eval";
    exec("rm " + g1TmpFile + " " + g2TmpFile + " " + evalFile + " " + alignmetFile);
}

Alignment HubAlignWrapper::run() {
    if (alpha > 0 and not fileExists(similarityFile)) {
        Sequence sequence(G1, G2);
        sequence.generateBitscoresFile(similarityFile);
    }
    if (not fileExists(g1EdgeListFile)) {
        generateEdgeListFile(1);
    }
    if (not fileExists(g2EdgeListFile)) {
        generateEdgeListFile(2);
    }
    copyEdgeListsToTmpFiles();
    generateAlignment();
    Alignment A = Alignment::loadEdgeList(G1, G2, alignmetFile);
    deleteAuxFiles();
    return A;
}

void HubAlignWrapper::describeParameters(ostream& stream) {
    stream << "alpha: " << alpha << endl;
    if (alpha > 0) {
        stream << "Similarity file: " << similarityFile << endl;
    }
}

string HubAlignWrapper::fileNameSuffix(const Alignment& A) {
    if (alpha == 1) return "_alpha_1";
    else return "_alpha_0" + extractDecimals(alpha,1);
}