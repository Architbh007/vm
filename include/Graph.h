#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <limits>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>

// Edge structure - represents a weighted connection between nodes
struct Edge {
    int destination;      // Target node
    double weight;        // Edge cost/distance
    
    Edge(int dest, double w) : destination(dest), weight(w) {}
};

// Node structure - represents a vertex in the graph
struct Node {
    int id;                          // Node identifier
    double x, y;                     // Coordinates (for A* heuristic)
    std::vector<Edge> adjacencyList; // Outgoing edges
    
    Node() : id(-1), x(0.0), y(0.0) {}
    Node(int nodeId, double xCoord = 0.0, double yCoord = 0.0) 
        : id(nodeId), x(xCoord), y(yCoord) {}
    
    // Add an edge from this node
    void addEdge(int dest, double weight) {
        adjacencyList.emplace_back(dest, weight);
    }
};

// Main Graph class
class Graph {
private:
    std::vector<Node> nodes;
    int nodeCount;
    int edgeCount;

public:
    // Constructor
    Graph(int numNodes = 0) : nodeCount(numNodes), edgeCount(0) {
        nodes.resize(numNodes);
        for (int i = 0; i < numNodes; i++) {
            nodes[i].id = i;
        }
    }
    
    // Get number of nodes
    int getNodeCount() const { 
        return nodeCount; 
    }
    
    // Get number of edges
    int getEdgeCount() const { 
        return edgeCount; 
    }
    
    // Set node count (resize graph)
    void setNodeCount(int n) {
        nodeCount = n;
        nodes.resize(n);
        for (int i = 0; i < n; i++) {
            nodes[i].id = i;
        }
    }
    
    // Set coordinates for a node (used for A* heuristic)
    void setNodeCoordinates(int nodeId, double x, double y) {
        if (nodeId >= 0 && nodeId < nodeCount) {
            nodes[nodeId].x = x;
            nodes[nodeId].y = y;
        }
    }
    
    // Add a directed edge
    void addEdge(int from, int to, double weight) {
        if (from >= 0 && from < nodeCount && to >= 0 && to < nodeCount) {
            nodes[from].addEdge(to, weight);
            edgeCount++;
        }
    }
    
    // Add a bidirectional edge (undirected graph)
    void addBidirectionalEdge(int from, int to, double weight) {
        addEdge(from, to, weight);
        addEdge(to, from, weight);
    }
    
    // Get a node reference
    const Node& getNode(int id) const {
        return nodes[id];
    }
    
    // Get adjacency list for a node
    const std::vector<Edge>& getAdjacencyList(int nodeId) const {
        return nodes[nodeId].adjacencyList;
    }
    
    // Calculate Euclidean distance heuristic (for A*)
    double getHeuristic(int fromNode, int toNode) const {
        if (fromNode < 0 || fromNode >= nodeCount || 
            toNode < 0 || toNode >= nodeCount) {
            return 0.0;
        }
        
        double dx = nodes[fromNode].x - nodes[toNode].x;
        double dy = nodes[fromNode].y - nodes[toNode].y;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    // Load graph from file
    // File format:
    // Line 1: numNodes numEdges
    // Following lines: fromNode toNode weight
    bool loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file " << filename << std::endl;
            return false;
        }
        
        int numNodes, numEdges;
        file >> numNodes >> numEdges;
        
        setNodeCount(numNodes);
        
        // Read edges
        for (int i = 0; i < numEdges; i++) {
            int from, to;
            double weight;
            file >> from >> to >> weight;
            addEdge(from, to, weight);
        }
        
        file.close();
        return true;
    }
    
    // Save graph to file
    bool saveToFile(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot create file " << filename << std::endl;
            return false;
        }
        
        file << nodeCount << " " << edgeCount << "\n";
        
        // Write all edges
        for (int i = 0; i < nodeCount; i++) {
            for (const Edge& edge : nodes[i].adjacencyList) {
                file << i << " " << edge.destination << " " << edge.weight << "\n";
            }
        }
        
        file.close();
        return true;
    }
    
    // Print graph info
    void printInfo() const {
        std::cout << "Graph Info:\n";
        std::cout << "  Nodes: " << nodeCount << "\n";
        std::cout << "  Edges: " << edgeCount << "\n";
        std::cout << "  Avg degree: " << (nodeCount > 0 ? (double)edgeCount / nodeCount : 0) << "\n";
    }
    
    // Get nodes in a partition (for distributed processing)
    std::vector<int> getPartition(int partitionId, int totalPartitions) const {
        std::vector<int> partition;
        
        // Contiguous partitioning
        int nodesPerPartition = nodeCount / totalPartitions;
        int remainder = nodeCount % totalPartitions;
        
        int startNode = partitionId * nodesPerPartition + std::min(partitionId, remainder);
        int endNode = startNode + nodesPerPartition + (partitionId < remainder ? 1 : 0);
        
        for (int i = startNode; i < endNode; i++) {
            partition.push_back(i);
        }
        
        return partition;
    }
};

// Priority queue element for Dijkstra/A*
struct PQElement {
    int nodeId;
    double distance;
    double fScore;  // f(n) = g(n) + h(n) for A*
    
    PQElement(int id, double dist, double f = 0.0) 
        : nodeId(id), distance(dist), fScore(f) {}
    
    // For priority queue (min-heap)
    bool operator>(const PQElement& other) const {
        return fScore > other.fScore;
    }
};

// Result structure for pathfinding
struct PathResult {
    std::vector<int> path;
    double totalDistance;
    bool found;
    double executionTime;  // in milliseconds
    
    PathResult() 
        : totalDistance(std::numeric_limits<double>::infinity()), 
          found(false), 
          executionTime(0.0) {}
    
    void printResult() const {
        if (found) {
            std::cout << "Path found!\n";
            std::cout << "Distance: " << totalDistance << "\n";
            std::cout << "Path length: " << path.size() << " nodes\n";
            std::cout << "Path: ";
            for (size_t i = 0; i < path.size(); i++) {
                std::cout << path[i];
                if (i < path.size() - 1) std::cout << " -> ";
            }
            std::cout << "\n";
        } else {
            std::cout << "No path found!\n";
        }
        std::cout << "Execution time: " << executionTime << " ms\n";
    }
};

#endif 
