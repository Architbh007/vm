#!/bin/bash

echo "=========================================="
echo "SIT315 Distributed Pathfinding Benchmark"
echo "Multi-VM Testing Configuration"
echo "=========================================="

# Configuration
SIZES=(10000 15000 20000 25000 30000)
LOCAL_PROCESSES=(2 4)
MULTI_VM_PROCESSES=(6 8)
REMOTE_VM="192.168.56.101"
OUTPUT="benchmark_results.csv"

# Step 1: Clean and Compile
echo ""
echo "Step 1: Cleaning and compiling..."
echo "----------------------------------------"
make clean
make
if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi
echo "✓ Compilation successful"

# Step 2: Generate Graphs
echo ""
echo "Step 2: Generating test graphs..."
echo "----------------------------------------"
mkdir -p data/synthetic
for SIZE in "${SIZES[@]}"; do
    EDGES=$((SIZE * 6))
    GRAPH="data/synthetic/graph_${SIZE}.txt"
    if [ ! -f "$GRAPH" ]; then
        echo "Generating $SIZE nodes with $EDGES edges..."
        ./build/generator $SIZE $EDGES $GRAPH
    else
        echo "Graph $SIZE already exists, skipping..."
    fi
done
echo "✓ All graphs ready"

# Step 3: Run Benchmarks
echo ""
echo "Step 3: Running benchmarks..."
echo "----------------------------------------"
echo "Implementation,Graph_Size,Processes,Time_ms,Distance,Iterations" > $OUTPUT

# Sequential Tests
echo ""
echo "=== SEQUENTIAL TESTS ==="
for SIZE in "${SIZES[@]}"; do
    GRAPH="data/synthetic/graph_${SIZE}.txt"
    SOURCE=0
    DEST=$((SIZE-1))
    
    echo "Testing Sequential: $SIZE nodes"
    RESULT=$(./build/sequential $GRAPH $SOURCE $DEST 2>/dev/null)
    TIME=$(echo "$RESULT" | grep "Execution time:" | awk '{print $3}')
    DIST=$(echo "$RESULT" | grep "Distance:" | awk '{print $2}')
    echo "Sequential,$SIZE,1,$TIME,$DIST,N/A" >> $OUTPUT
    echo "  Sequential: ${TIME}ms, Distance: $DIST"
done

# Local Distributed Tests (2, 4 processors)
echo ""
echo "=== LOCAL DISTRIBUTED TESTS (2, 4 Processors) ==="
for SIZE in "${SIZES[@]}"; do
    GRAPH="data/synthetic/graph_${SIZE}.txt"
    SOURCE=0
    DEST=$((SIZE-1))
    
    echo "Testing Distributed: $SIZE nodes"
    
    for NP in "${LOCAL_PROCESSES[@]}"; do
        echo "  [$NP processors - local]"
        RESULT=$(mpirun -np $NP ./build/distributed $GRAPH $SOURCE $DEST 2>/dev/null)
        TIME=$(echo "$RESULT" | grep "Execution time:" | awk '{print $3}')
        DIST=$(echo "$RESULT" | grep "Distance:" | awk '{print $2}')
        ITERS=$(echo "$RESULT" | grep "Iterations:" | awk '{print $2}')
        echo "Distributed-Local,$SIZE,$NP,$TIME,$DIST,$ITERS" >> $OUTPUT
        echo "    Time: ${TIME}ms, Distance: $DIST, Iterations: $ITERS"
    done
done

# Multi-VM Distributed Tests (6, 8 processors)
echo ""
echo "=== MULTI-VM DISTRIBUTED TESTS (6, 8 Processors) ==="
echo "Using VMs: localhost + $REMOTE_VM"

# Check if remote VM is accessible
ssh -o ConnectTimeout=5 $USER@$REMOTE_VM "echo 'Remote VM accessible'" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "WARNING: Cannot connect to $REMOTE_VM"
    echo "Skipping multi-VM tests. Check:"
    echo "  1. Remote VM is running"
    echo "  2. SSH access is configured"
    echo "  3. MPI is installed on remote VM"
else
    # Create hostfile for MPI
    HOSTFILE="mpi_hosts.txt"
    echo "localhost slots=4" > $HOSTFILE
    echo "$REMOTE_VM slots=4" >> $HOSTFILE
    
    for SIZE in "${SIZES[@]}"; do
        GRAPH="data/synthetic/graph_${SIZE}.txt"
        SOURCE=0
        DEST=$((SIZE-1))
        
        echo "Testing Multi-VM Distributed: $SIZE nodes"
        
        for NP in "${MULTI_VM_PROCESSES[@]}"; do
            echo "  [$NP processors - multi-VM]"
            RESULT=$(mpirun -np $NP --hostfile $HOSTFILE ./build/distributed $GRAPH $SOURCE $DEST 2>/dev/null)
            TIME=$(echo "$RESULT" | grep "Execution time:" | awk '{print $3}')
            DIST=$(echo "$RESULT" | grep "Distance:" | awk '{print $2}')
            ITERS=$(echo "$RESULT" | grep "Iterations:" | awk '{print $2}')
            echo "Distributed-MultiVM,$SIZE,$NP,$TIME,$DIST,$ITERS" >> $OUTPUT
            echo "    Time: ${TIME}ms, Distance: $DIST, Iterations: $ITERS"
        done
    done
    
    rm -f $HOSTFILE
fi

echo ""
echo "=========================================="
echo "Benchmark Complete!"
echo "=========================================="
echo ""
echo "Results saved to: $OUTPUT"
echo ""
echo "Summary Table:"
echo "=========================================="
column -t -s',' $OUTPUT

# Calculate speedups
echo ""
echo "=========================================="
echo "Speedup Analysis:"
echo "=========================================="

for SIZE in "${SIZES[@]}"; do
    SEQ_TIME=$(grep "Sequential,$SIZE,1" $OUTPUT | cut -d',' -f4)
    
    if [ ! -z "$SEQ_TIME" ] && [ "$SEQ_TIME" != "0" ]; then
        echo ""
        echo "$SIZE nodes (Sequential: ${SEQ_TIME}ms):"
        
        # Local processors
        for NP in "${LOCAL_PROCESSES[@]}"; do
            DIST_TIME=$(grep "Distributed-Local,$SIZE,$NP" $OUTPUT | cut -d',' -f4)
            if [ ! -z "$DIST_TIME" ] && [ "$DIST_TIME" != "0" ]; then
                SPEEDUP=$(echo "scale=2; $SEQ_TIME / $DIST_TIME" | bc)
                EFFICIENCY=$(echo "scale=1; ($SPEEDUP / $NP) * 100" | bc)
                echo "  $NP processes (local): ${DIST_TIME}ms → ${SPEEDUP}x speedup (${EFFICIENCY}% efficiency)"
            fi
        done
        
        # Multi-VM processors
        for NP in "${MULTI_VM_PROCESSES[@]}"; do
            DIST_TIME=$(grep "Distributed-MultiVM,$SIZE,$NP" $OUTPUT | cut -d',' -f4)
            if [ ! -z "$DIST_TIME" ] && [ "$DIST_TIME" != "0" ]; then
                SPEEDUP=$(echo "scale=2; $SEQ_TIME / $DIST_TIME" | bc)
                EFFICIENCY=$(echo "scale=1; ($SPEEDUP / $NP) * 100" | bc)
                echo "  $NP processes (multi-VM): ${DIST_TIME}ms → ${SPEEDUP}x speedup (${EFFICIENCY}% efficiency)"
            fi
        done
    fi
done

echo ""
echo "=========================================="
echo "Note: Multi-VM tests require:"
echo "  1. SSH access to $REMOTE_VM"
echo "  2. MPI installed on both VMs"
echo "  3. Same project structure on both VMs"
echo "  4. SSH keys configured for passwordless access"
echo "=========================================="
