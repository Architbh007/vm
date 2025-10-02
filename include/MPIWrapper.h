#ifndef MPI_WRAPPER_H
#define MPI_WRAPPER_H

#include <mpi.h>
#include "Graph.h"
#include <vector>
#include <iostream>

// MPI message tags
namespace MPITags {
    const int GRAPH_DATA = 1;
    const int PARTITION_SIZE = 2;
    const int PARTITION_NODES = 3;
    const int DISTANCE_UPDATE = 4;
    const int PATH_REQUEST = 5;
    const int PATH_RESPONSE = 6;
    const int WORK_STEAL_REQUEST = 7;
    const int WORK_STEAL_RESPONSE = 8;
    const int TERMINATE = 9;
}

// Structure for distance updates between processes
struct DistanceUpdate {
    int nodeId;
    double distance;
    int fromProcess;
    
    DistanceUpdate() : nodeId(-1), distance(0.0), fromProcess(-1) {}
    DistanceUpdate(int id, double dist, int from) 
        : nodeId(id), distance(dist), fromProcess(from) {}
};

// MPI Wrapper class with helper functions
class MPIWrapper {
public:
    // Send a partition of nodes to a worker process
    static void sendPartition(
        const std::vector<int>& partition,
        int destinationRank
    ) {
        int partitionSize = partition.size();
        
        // Send partition size first
        MPI_Send(&partitionSize, 1, MPI_INT, destinationRank, 
                 MPITags::PARTITION_SIZE, MPI_COMM_WORLD);
        
        // Send node IDs
        if (partitionSize > 0) {
            MPI_Send(partition.data(), partitionSize, MPI_INT, 
                     destinationRank, MPITags::PARTITION_NODES, MPI_COMM_WORLD);
        }
    }
    
    // Receive partition assignment from master
    static std::vector<int> receivePartition(int sourceRank) {
        int partitionSize;
        MPI_Status status;
        
        // Receive partition size
        MPI_Recv(&partitionSize, 1, MPI_INT, sourceRank, 
                 MPITags::PARTITION_SIZE, MPI_COMM_WORLD, &status);
        
        // Receive node IDs
        std::vector<int> partition(partitionSize);
        if (partitionSize > 0) {
            MPI_Recv(partition.data(), partitionSize, MPI_INT, 
                     sourceRank, MPITags::PARTITION_NODES, MPI_COMM_WORLD, &status);
        }
        
        return partition;
    }
    
    // Broadcast graph data to all processes
    static void broadcastGraphData(
        Graph& graph,
        int rootRank,
        int myRank
    ) {
        int nodeCount = graph.getNodeCount();
        int edgeCount = graph.getEdgeCount();
        
        // Broadcast node and edge counts
        MPI_Bcast(&nodeCount, 1, MPI_INT, rootRank, MPI_COMM_WORLD);
        MPI_Bcast(&edgeCount, 1, MPI_INT, rootRank, MPI_COMM_WORLD);
        
        // Non-root processes initialize graph
        if (myRank != rootRank) {
            graph.setNodeCount(nodeCount);
        }
        
        // Broadcast edges
        std::vector<int> edgeData;
        std::vector<double> weightData;
        
        if (myRank == rootRank) {
            // Root prepares edge data
            for (int i = 0; i < nodeCount; i++) {
                const std::vector<Edge>& edges = graph.getAdjacencyList(i);
                for (const Edge& edge : edges) {
                    edgeData.push_back(i);
                    edgeData.push_back(edge.destination);
                    weightData.push_back(edge.weight);
                }
            }
        }
        
        // Broadcast actual edge count (in case it differs)
        int actualEdgeCount = edgeData.size() / 2;
        MPI_Bcast(&actualEdgeCount, 1, MPI_INT, rootRank, MPI_COMM_WORLD);
        
        // Resize vectors for non-root processes
        if (myRank != rootRank) {
            edgeData.resize(actualEdgeCount * 2);
            weightData.resize(actualEdgeCount);
        }
        
        // Broadcast edge data
        if (actualEdgeCount > 0) {
            MPI_Bcast(edgeData.data(), actualEdgeCount * 2, MPI_INT, 
                      rootRank, MPI_COMM_WORLD);
            MPI_Bcast(weightData.data(), actualEdgeCount, MPI_DOUBLE, 
                      rootRank, MPI_COMM_WORLD);
        }
        
        // Non-root processes reconstruct graph
        if (myRank != rootRank) {
            for (int i = 0; i < actualEdgeCount; i++) {
                int from = edgeData[i * 2];
                int to = edgeData[i * 2 + 1];
                double weight = weightData[i];
                graph.addEdge(from, to, weight);
            }
        }
    }
    
    // Send distance update to another process
    static void sendDistanceUpdate(
        int nodeId,
        double distance,
        int destinationRank,
        int myRank
    ) {
        DistanceUpdate update(nodeId, distance, myRank);
        MPI_Send(&update, sizeof(DistanceUpdate), MPI_BYTE, 
                 destinationRank, MPITags::DISTANCE_UPDATE, MPI_COMM_WORLD);
    }
    
    // Non-blocking receive of distance updates
    static bool receiveDistanceUpdate(
        DistanceUpdate& update,
        MPI_Request* request = nullptr
    ) {
        MPI_Status status;
        int flag;
        
        // Check if message is available
        MPI_Iprobe(MPI_ANY_SOURCE, MPITags::DISTANCE_UPDATE, 
                   MPI_COMM_WORLD, &flag, &status);
        
        if (flag) {
            MPI_Recv(&update, sizeof(DistanceUpdate), MPI_BYTE, 
                     status.MPI_SOURCE, MPITags::DISTANCE_UPDATE, 
                     MPI_COMM_WORLD, &status);
            return true;
        }
        
        return false;
    }
    
    // Barrier synchronization
    static void barrier() {
        MPI_Barrier(MPI_COMM_WORLD);
    }
    
    // Reduce: collect minimum distance
    static double reduceMinDistance(double localMin, int rootRank) {
        double globalMin;
        MPI_Reduce(&localMin, &globalMin, 1, MPI_DOUBLE, 
                   MPI_MIN, rootRank, MPI_COMM_WORLD);
        return globalMin;
    }
    
    // Gather: collect all distances
    static std::vector<double> gatherDistances(
        const std::vector<double>& localDistances,
        int rootRank,
        int myRank
    ) {
        int localSize = localDistances.size();
        std::vector<int> allSizes;
        
        if (myRank == rootRank) {
            int worldSize;
            MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
            allSizes.resize(worldSize);
        }
        
        // Gather sizes
        MPI_Gather(&localSize, 1, MPI_INT, 
                   allSizes.data(), 1, MPI_INT, 
                   rootRank, MPI_COMM_WORLD);
        
        // Prepare displacement array
        std::vector<int> displacements;
        int totalSize = 0;
        
        if (myRank == rootRank) {
            displacements.resize(allSizes.size());
            for (size_t i = 0; i < allSizes.size(); i++) {
                displacements[i] = totalSize;
                totalSize += allSizes[i];
            }
        }
        
        // Gather actual data
        std::vector<double> allDistances;
        if (myRank == rootRank) {
            allDistances.resize(totalSize);
        }
        
        MPI_Gatherv(localDistances.data(), localSize, MPI_DOUBLE,
                    allDistances.data(), allSizes.data(), 
                    displacements.data(), MPI_DOUBLE,
                    rootRank, MPI_COMM_WORLD);
        
        return allDistances;
    }
    
    // Broadcast termination signal
    static void broadcastTerminate(int rootRank) {
        int terminateSignal = 1;
        MPI_Bcast(&terminateSignal, 1, MPI_INT, rootRank, MPI_COMM_WORLD);
    }
    
    // Check for termination signal
    static bool checkTerminate() {
        int flag;
        MPI_Status status;
        MPI_Iprobe(MPI_ANY_SOURCE, MPITags::TERMINATE, 
                   MPI_COMM_WORLD, &flag, &status);
        return flag;
    }
    
    // Get MPI rank and size
    static void getMPIInfo(int& rank, int& size) {
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);
    }
    
    // Print process information
    static void printProcessInfo(int rank, int size) {
        std::cout << "Process " << rank << " of " << size << " initialized\n";
    }
};

#endif // MPI_WRAPPER_H
