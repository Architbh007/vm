#ifndef PARTITIONER_H
#define PARTITIONER_H

#include "Graph.h"
#include <vector>
#include <unordered_set>

class Partitioner {
public:
    // Existing contiguous partitioning
    static std::vector<int> getContiguousPartition(
        const Graph& graph,
        int myRank,
        int totalProcesses
    ) {
        std::vector<int> partition;
        int nodeCount = graph.getNodeCount();
        
        // Calculate nodes per process
        int nodesPerProcess = nodeCount / totalProcesses;
        int remainder = nodeCount % totalProcesses;
        
        // Calculate start and end for this rank
        int startNode = myRank * nodesPerProcess + std::min(myRank, remainder);
        int endNode = startNode + nodesPerProcess + (myRank < remainder ? 1 : 0);
        
        // Add nodes to partition
        for (int i = startNode; i < endNode; i++) {
            partition.push_back(i);
        }
        
        return partition;
    }
    
    // NEW: Round-Robin partitioning
    static std::vector<int> getRoundRobinPartition(
        const Graph& graph,
        int myRank,
        int totalProcesses
    ) {
        std::vector<int> partition;
        int nodeCount = graph.getNodeCount();
        
        // Assign nodes in round-robin fashion
        // Process 0: 0, 4, 8, 12, ...
        // Process 1: 1, 5, 9, 13, ...
        // Process 2: 2, 6, 10, 14, ...
        // Process 3: 3, 7, 11, 15, ...
        for (int node = myRank; node < nodeCount; node += totalProcesses) {
            partition.push_back(node);
        }
        
        return partition;
    }
    
    // Helper: Get owner of a node (for round-robin)
    static int getNodeOwnerRoundRobin(int nodeId, int totalProcesses) {
        return nodeId % totalProcesses;
    }
    
    // Helper: Get owner of a node (for contiguous)
    static int getNodeOwnerContiguous(int nodeId, int nodeCount, int totalProcesses) {
        int nodesPerProcess = nodeCount / totalProcesses;
        int remainder = nodeCount % totalProcesses;
        
        // Handle uneven distribution
        int accumulatedNodes = 0;
        for (int rank = 0; rank < totalProcesses; rank++) {
            int rankNodes = nodesPerProcess + (rank < remainder ? 1 : 0);
            if (nodeId < accumulatedNodes + rankNodes) {
                return rank;
            }
            accumulatedNodes += rankNodes;
        }
        return totalProcesses - 1;
    }
    
    // Identify boundary nodes (nodes with edges to other partitions)
    static std::unordered_set<int> identifyBoundaryNodes(
        const Graph& graph,
        const std::vector<int>& myPartition,
        int myRank,
        int totalProcesses
    ) {
        std::unordered_set<int> boundaryNodes;
        std::unordered_set<int> myNodes(myPartition.begin(), myPartition.end());
        
        for (int node : myPartition) {
            for (const Edge& edge : graph.getAdjacencyList(node)) {
                // If edge goes to node not in my partition, it's a boundary
                if (myNodes.find(edge.destination) == myNodes.end()) {
                    boundaryNodes.insert(node);
                    break;
                }
            }
        }
        
        return boundaryNodes;
    }
};

#endif
