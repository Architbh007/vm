#include "../include/Graph.h"
#include <iostream>
#include <random>
#include <ctime>
#include <cstring>

using namespace std;

// Generate a random connected graph
void generateRandomGraph(
    int numNodes,
    int numEdges,
    const string& filename,
    double minWeight = 1.0,
    double maxWeight = 100.0
) {
    Graph graph(numNodes);
    
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> weightDist(minWeight, maxWeight);
    uniform_real_distribution<> coordDist(0.0, 1000.0);
    uniform_int_distribution<> nodeDist(0, numNodes - 1);
    
    // Assign random coordinates to all nodes (for A* heuristic)
    for (int i = 0; i < numNodes; i++) {
        double x = coordDist(gen);
        double y = coordDist(gen);
        graph.setNodeCoordinates(i, x, y);
    }
    
    // First, create a connected graph (spanning tree)
    cout << "Creating connected graph backbone...\n";
    for (int i = 1; i < numNodes; i++) {
        int parent = nodeDist(gen) % i;  // Connect to a previous node
        double weight = weightDist(gen);
        graph.addBidirectionalEdge(parent, i, weight);
    }
    
    // Add remaining edges randomly
    int edgesAdded = numNodes - 1;  // Already added spanning tree edges
    int attempts = 0;
    int maxAttempts = numEdges * 10;
    
    cout << "Adding additional edges...\n";
    while (edgesAdded < numEdges && attempts < maxAttempts) {
        int from = nodeDist(gen);
        int to = nodeDist(gen);
        
        // Avoid self-loops
        if (from != to) {
            double weight = weightDist(gen);
            graph.addBidirectionalEdge(from, to, weight);
            edgesAdded += 2;  // Bidirectional adds 2 edges
        }
        
        attempts++;
    }
    
    // Save to file
    cout << "Saving graph to " << filename << "...\n";
    if (graph.saveToFile(filename)) {
        cout << "✓ Graph generated successfully!\n";
        graph.printInfo();
    } else {
        cerr << "✗ Error saving graph to file!\n";
    }
}

// Generate a grid graph (useful for testing)
void generateGridGraph(
    int rows,
    int cols,
    const string& filename,
    double edgeWeight = 1.0
) {
    int numNodes = rows * cols;
    Graph graph(numNodes);
    
    cout << "Generating " << rows << "x" << cols << " grid graph...\n";
    
    // Assign grid coordinates
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int nodeId = r * cols + c;
            graph.setNodeCoordinates(nodeId, c * 10.0, r * 10.0);
        }
    }
    
    // Add edges (4-connected grid)
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int nodeId = r * cols + c;
            
            // Right neighbor
            if (c < cols - 1) {
                int neighbor = r * cols + (c + 1);
                graph.addBidirectionalEdge(nodeId, neighbor, edgeWeight);
            }
            
            // Bottom neighbor
            if (r < rows - 1) {
                int neighbor = (r + 1) * cols + c;
                graph.addBidirectionalEdge(nodeId, neighbor, edgeWeight);
            }
        }
    }
    
    // Save to file
    cout << "Saving grid graph to " << filename << "...\n";
    if (graph.saveToFile(filename)) {
        cout << "✓ Grid graph generated successfully!\n";
        graph.printInfo();
    } else {
        cerr << "✗ Error saving graph to file!\n";
    }
}

// Print usage information
void printUsage(const char* programName) {
    cout << "Graph Generator - Create synthetic test graphs\n\n";
    cout << "Usage:\n";
    cout << "  Random graph: " << programName << " <nodes> <edges> <output_file> [min_weight] [max_weight]\n";
    cout << "  Grid graph:   " << programName << " --grid <rows> <cols> <output_file> [edge_weight]\n";
    cout << "\nExamples:\n";
    cout << "  " << programName << " 1000 5000 data/synthetic/graph_1000.txt\n";
    cout << "  " << programName << " 1000 5000 data/synthetic/graph_1000.txt 1.0 100.0\n";
    cout << "  " << programName << " --grid 50 50 data/synthetic/grid_50x50.txt\n";
    cout << "  " << programName << " --grid 50 50 data/synthetic/grid_50x50.txt 2.5\n";
}

int main(int argc, char* argv[]) {
    // Check for help flag
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        printUsage(argv[0]);
        return 0;
    }
    
    // Check for grid graph generation
    if (strcmp(argv[1], "--grid") == 0) {
        if (argc < 5) {
            cerr << "Error: Grid graph requires <rows> <cols> <output_file>\n";
            printUsage(argv[0]);
            return 1;
        }
        
        int rows = atoi(argv[2]);
        int cols = atoi(argv[3]);
        string filename = argv[4];
        double edgeWeight = (argc >= 6) ? atof(argv[5]) : 1.0;
        
        if (rows <= 0 || cols <= 0) {
            cerr << "Error: Invalid grid dimensions\n";
            return 1;
        }
        
        generateGridGraph(rows, cols, filename, edgeWeight);
        return 0;
    }
    
    // Random graph generation
    if (argc < 4) {
        cerr << "Error: Random graph requires <nodes> <edges> <output_file>\n";
        printUsage(argv[0]);
        return 1;
    }
    
    int numNodes = atoi(argv[1]);
    int numEdges = atoi(argv[2]);
    string filename = argv[3];
    double minWeight = (argc >= 5) ? atof(argv[4]) : 1.0;
    double maxWeight = (argc >= 6) ? atof(argv[5]) : 100.0;
    
    // Validate inputs
    if (numNodes <= 0) {
        cerr << "Error: Number of nodes must be positive\n";
        return 1;
    }
    
    if (numEdges < numNodes - 1) {
        cerr << "Error: Number of edges must be at least (nodes - 1) for connectivity\n";
        return 1;
    }
    
    int maxPossibleEdges = numNodes * (numNodes - 1);  // Directed edges
    if (numEdges > maxPossibleEdges) {
        cerr << "Warning: Number of edges exceeds maximum possible. Using maximum.\n";
        numEdges = maxPossibleEdges;
    }
    
    if (minWeight >= maxWeight) {
        cerr << "Error: min_weight must be less than max_weight\n";
        return 1;
    }
    
    // Generate the graph
    cout << "===========================================\n";
    cout << "Graph Generator\n";
    cout << "===========================================\n";
    cout << "Nodes:       " << numNodes << "\n";
    cout << "Edges:       " << numEdges << "\n";
    cout << "Output:      " << filename << "\n";
    cout << "Weight range: [" << minWeight << ", " << maxWeight << "]\n";
    cout << "===========================================\n\n";
    
    generateRandomGraph(numNodes, numEdges, filename, minWeight, maxWeight);
    
    cout << "\n===========================================\n";
    cout << "Generation complete!\n";
    cout << "===========================================\n";
    
    return 0;
}
