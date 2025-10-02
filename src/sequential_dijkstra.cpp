#include "../include/Graph.h"
#include <queue>
#include <vector>
#include <limits>
#include <iostream>
#include <chrono>

using namespace std;

// Sequential Dijkstra's Algorithm
PathResult sequentialDijkstra(const Graph& graph, int source, int destination) {
    PathResult result;
    int nodeCount = graph.getNodeCount();
    
    // Initialize distances and predecessors
    vector<double> distances(nodeCount, numeric_limits<double>::infinity());
    vector<int> predecessors(nodeCount, -1);
    vector<bool> visited(nodeCount, false);
    
    // Priority queue: min-heap based on distance
    priority_queue<PQElement, vector<PQElement>, greater<PQElement>> pq;
    
    // Start from source
    distances[source] = 0.0;
    pq.push(PQElement(source, 0.0, 0.0));
    
    int nodesProcessed = 0;
    
    while (!pq.empty()) {
        PQElement current = pq.top();
        pq.pop();
        
        int currentNode = current.nodeId;
        
        // Skip if already visited
        if (visited[currentNode]) {
            continue;
        }
        
        visited[currentNode] = true;
        nodesProcessed++;
        
        // Early termination if destination reached
        if (currentNode == destination) {
            break;
        }
        
        // Explore neighbors
        const vector<Edge>& edges = graph.getAdjacencyList(currentNode);
        
        for (const Edge& edge : edges) {
            int neighbor = edge.destination;
            double newDistance = distances[currentNode] + edge.weight;
            
            // Relaxation step
            if (newDistance < distances[neighbor]) {
                distances[neighbor] = newDistance;
                predecessors[neighbor] = currentNode;
                pq.push(PQElement(neighbor, newDistance, newDistance));
            }
        }
    }
    
    // Reconstruct path
    if (distances[destination] != numeric_limits<double>::infinity()) {
        result.found = true;
        result.totalDistance = distances[destination];
        
        // Build path from destination to source
        vector<int> reversePath;
        int current = destination;
        
        while (current != -1) {
            reversePath.push_back(current);
            current = predecessors[current];
        }
        
        // Reverse to get path from source to destination
        result.path.assign(reversePath.rbegin(), reversePath.rend());
    }
    
    return result;
}

// Sequential Dijkstra with A* heuristic
PathResult sequentialAStarDijkstra(const Graph& graph, int source, int destination) {
    PathResult result;
    int nodeCount = graph.getNodeCount();
    
    // Initialize distances and predecessors
    vector<double> distances(nodeCount, numeric_limits<double>::infinity());
    vector<int> predecessors(nodeCount, -1);
    vector<bool> visited(nodeCount, false);
    
    // Priority queue: min-heap based on f-score (distance + heuristic)
    priority_queue<PQElement, vector<PQElement>, greater<PQElement>> pq;
    
    // Start from source
    distances[source] = 0.0;
    double heuristic = graph.getHeuristic(source, destination);
    pq.push(PQElement(source, 0.0, heuristic));
    
    int nodesProcessed = 0;
    
    while (!pq.empty()) {
        PQElement current = pq.top();
        pq.pop();
        
        int currentNode = current.nodeId;
        
        // Skip if already visited
        if (visited[currentNode]) {
            continue;
        }
        
        visited[currentNode] = true;
        nodesProcessed++;
        
        // Early termination if destination reached
        if (currentNode == destination) {
            break;
        }
        
        // Explore neighbors
        const vector<Edge>& edges = graph.getAdjacencyList(currentNode);
        
        for (const Edge& edge : edges) {
            int neighbor = edge.destination;
            double newDistance = distances[currentNode] + edge.weight;
            
            // Relaxation step
            if (newDistance < distances[neighbor]) {
                distances[neighbor] = newDistance;
                predecessors[neighbor] = currentNode;
                
                // Calculate f-score: g(n) + h(n)
                double heuristic = graph.getHeuristic(neighbor, destination);
                double fScore = newDistance + heuristic;
                
                pq.push(PQElement(neighbor, newDistance, fScore));
            }
        }
    }
    
    // Reconstruct path
    if (distances[destination] != numeric_limits<double>::infinity()) {
        result.found = true;
        result.totalDistance = distances[destination];
        
        // Build path from destination to source
        vector<int> reversePath;
        int current = destination;
        
        while (current != -1) {
            reversePath.push_back(current);
            current = predecessors[current];
        }
        
        // Reverse to get path from source to destination
        result.path.assign(reversePath.rbegin(), reversePath.rend());
    }
    
    return result;
}

void printUsage(const char* programName) {
    cout << "Sequential Dijkstra - Baseline Shortest Path Finder\n\n";
    cout << "Usage:\n";
    cout << "  " << programName << " <graph_file> <source> <destination> [--astar]\n";
    cout << "\nArguments:\n";
    cout << "  graph_file    - Path to graph data file\n";
    cout << "  source        - Source node ID\n";
    cout << "  destination   - Destination node ID\n";
    cout << "  --astar       - Use A* heuristic (optional)\n";
    cout << "\nExample:\n";
    cout << "  " << programName << " data/synthetic/graph_1000.txt 0 999\n";
    cout << "  " << programName << " data/synthetic/graph_1000.txt 0 999 --astar\n";
}

int main(int argc, char* argv[]) {
    // Check arguments
    if (argc < 4) {
        printUsage(argv[0]);
        return 1;
    }
    
    string graphFile = argv[1];
    int source = atoi(argv[2]);
    int destination = atoi(argv[3]);
    bool useAStar = (argc >= 5 && string(argv[4]) == "--astar");
    
    // Load graph
    cout << "===========================================\n";
    cout << "Sequential Dijkstra's Algorithm\n";
    cout << "===========================================\n";
    cout << "Loading graph from: " << graphFile << "\n";
    
    Graph graph;
    if (!graph.loadFromFile(graphFile)) {
        cerr << "Error: Failed to load graph file\n";
        return 1;
    }
    
    cout << "✓ Graph loaded successfully\n";
    graph.printInfo();
    
    // Validate source and destination
    if (source < 0 || source >= graph.getNodeCount() ||
        destination < 0 || destination >= graph.getNodeCount()) {
        cerr << "Error: Invalid source or destination node\n";
        return 1;
    }
    
    cout << "\nSource:      " << source << "\n";
    cout << "Destination: " << destination << "\n";
    cout << "Algorithm:   " << (useAStar ? "Dijkstra + A*" : "Standard Dijkstra") << "\n";
    cout << "===========================================\n\n";
    
    // Run algorithm
    cout << "Computing shortest path...\n";
    
    auto startTime = chrono::high_resolution_clock::now();
    
    PathResult result;
    if (useAStar) {
        result = sequentialAStarDijkstra(graph, source, destination);
    } else {
        result = sequentialDijkstra(graph, source, destination);
    }
    
    auto endTime = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
    
    result.executionTime = duration.count();
    
    // Display results
    cout << "\n===========================================\n";
    cout << "Results\n";
    cout << "===========================================\n";
    result.printResult();
    cout << "===========================================\n";
    
    return 0;
}
