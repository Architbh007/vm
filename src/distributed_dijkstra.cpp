#include "../include/Graph.h"
#include "../include/Partitioner.h"
#include <mpi.h>
#include <vector>
#include <limits>
#include <iostream>
#include <chrono>
#include <unordered_set>

using namespace std;
using namespace std::chrono;

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (argc < 4) {
        if (rank == 0) {
            cout << "Usage: mpirun -np N " << argv[0] << " graph.txt source dest\n";
        }
        MPI_Finalize();
        return 1;
    }
    
    string graphFile = argv[1];
    int source = atoi(argv[2]);
    int destination = atoi(argv[3]);
    
    // All processes load graph
    Graph graph;
    if (!graph.loadFromFile(graphFile)) {
        cerr << "Process " << rank << ": Error loading graph\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    int nodeCount = graph.getNodeCount();
    int edgeCount = graph.getEdgeCount();
    
    // Round-robin partitioning
    vector<int> myPartition = Partitioner::getRoundRobinPartition(graph, rank, size);
    unordered_set<int> myNodes(myPartition.begin(), myPartition.end());
    
    MPI_Barrier(MPI_COMM_WORLD);
    auto startTime = high_resolution_clock::now();
    
    // Initialize distances
    vector<double> distances(nodeCount, numeric_limits<double>::infinity());
    vector<double> globalDistances(nodeCount, numeric_limits<double>::infinity());
    
    // Set source distance
    if (myNodes.count(source)) {
        distances[source] = 0.0;
    }
    
    // Initial synchronization
    MPI_Allreduce(distances.data(), globalDistances.data(), nodeCount, 
                  MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
    distances = globalDistances;
    
    // Statistics
    int iterationsCompleted = 0;
    int edgesRelaxed = 0;
    int localUpdates = 0;
    
    // Bellman-Ford style relaxation
    int maxIterations = nodeCount;
    
    for (int iteration = 0; iteration < maxIterations; iteration++) {
        bool localChanged = false;
        iterationsCompleted++;
        
        // Each process relaxes edges from its partition
        for (int u : myPartition) {
            if (distances[u] == numeric_limits<double>::infinity()) {
                continue;
            }
            
            for (const Edge& e : graph.getAdjacencyList(u)) {
                int v = e.destination;
                double newDist = distances[u] + e.weight;
                edgesRelaxed++;
                
                if (newDist < distances[v]) {
                    distances[v] = newDist;
                    localChanged = true;
                    localUpdates++;
                }
            }
        }
        
        // Synchronize distances across all processes
        MPI_Allreduce(distances.data(), globalDistances.data(), nodeCount, 
                      MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
        distances = globalDistances;
        
        // Check global convergence
        int localFlag = localChanged ? 1 : 0;
        int globalFlag = 0;
        MPI_Allreduce(&localFlag, &globalFlag, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        
        if (globalFlag == 0) {
            break;
        }
    }
    
    auto endTime = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(endTime - startTime).count();
    
    // Gather statistics
    int totalEdgesRelaxed, totalLocalUpdates, maxIterations_global;
    MPI_Reduce(&edgesRelaxed, &totalEdgesRelaxed, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&localUpdates, &totalLocalUpdates, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&iterationsCompleted, &maxIterations_global, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
    
    if (rank == 0) {
        double finalDist = distances[destination];
        
        cout << "===========================================\n";
        cout << "Distributed Dijkstra (BSP Model)\n";
        cout << "===========================================\n";
        cout << "Graph Statistics:\n";
        cout << "  Nodes: " << nodeCount << "\n";
        cout << "  Edges: " << edgeCount << "\n";
        cout << "-------------------------------------------\n";
        cout << "Parallel Configuration:\n";
        cout << "  Partitioning: Round-Robin\n";
        cout << "  Processes: " << size << "\n";
        cout << "  Nodes per process: ~" << (nodeCount / size) << "\n";
        cout << "-------------------------------------------\n";
        cout << "Results:\n";
        cout << "  Source: " << source << "\n";
        cout << "  Destination: " << destination << "\n";
        cout << "  Distance: " << finalDist << "\n";
        cout << "-------------------------------------------\n";
        cout << "Performance:\n";
        cout << "  Execution time: " << duration << " ms\n";
        cout << "  Iterations: " << maxIterations_global << "\n";
        cout << "  Total edges relaxed: " << totalEdgesRelaxed << "\n";
        cout << "  Distance updates: " << totalLocalUpdates << "\n";
        cout << "===========================================\n";
    }
    
    MPI_Finalize();
    return 0;
}
