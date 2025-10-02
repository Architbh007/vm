import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Read data
df = pd.read_csv('benchmark_results.csv')

print("\n" + "="*70)
print("DATA VERIFICATION")
print("="*70)
print(df.to_string(index=False))
print("="*70 + "\n")

# Create figure with subplots
fig = plt.figure(figsize=(18, 12))
gs = fig.add_gridspec(3, 3, hspace=0.3, wspace=0.3)

fig.suptitle('Distributed Pathfinding: Performance Analysis\n(Local vs Multi-VM Comparison)', 
             fontsize=16, fontweight='bold', y=0.98)

# Color scheme
colors = {
    'Sequential': '#2E86AB',
    'Local-2': '#A23B72',
    'Local-4': '#F18F01',
    'MultiVM-6': '#06A77D',
    'MultiVM-8': '#C73E1D'
}

graph_sizes = df['Graph_Size'].unique()
graph_sizes.sort()

# ===== Plot 1: Execution Time Comparison =====
ax1 = fig.add_subplot(gs[0, :])
x = np.arange(len(graph_sizes))
width = 0.15

# Sequential
seq_times = [df[(df['Implementation'] == 'Sequential') & 
                (df['Graph_Size'] == s)]['Time_ms'].values[0] for s in graph_sizes]
ax1.bar(x - 2*width, seq_times, width, label='Sequential', color=colors['Sequential'])

# Local 2P
local2_times = [df[(df['Implementation'] == 'Distributed-Local') & 
                   (df['Graph_Size'] == s) & 
                   (df['Processes'] == 2)]['Time_ms'].values[0] for s in graph_sizes]
ax1.bar(x - width, local2_times, width, label='Local 2P', color=colors['Local-2'])

# Local 4P
local4_times = [df[(df['Implementation'] == 'Distributed-Local') & 
                   (df['Graph_Size'] == s) & 
                   (df['Processes'] == 4)]['Time_ms'].values[0] for s in graph_sizes]
ax1.bar(x, local4_times, width, label='Local 4P', color=colors['Local-4'])

# Multi-VM 6P
multi6_times = [df[(df['Implementation'] == 'Distributed-MultiVM') & 
                   (df['Graph_Size'] == s) & 
                   (df['Processes'] == 6)]['Time_ms'].values[0] for s in graph_sizes]
ax1.bar(x + width, multi6_times, width, label='Multi-VM 6P', color=colors['MultiVM-6'])

# Multi-VM 8P
multi8_times = [df[(df['Implementation'] == 'Distributed-MultiVM') & 
                   (df['Graph_Size'] == s) & 
                   (df['Processes'] == 8)]['Time_ms'].values[0] for s in graph_sizes]
ax1.bar(x + 2*width, multi8_times, width, label='Multi-VM 8P', color=colors['MultiVM-8'])

ax1.set_xlabel('Graph Size (nodes)', fontsize=11)
ax1.set_ylabel('Execution Time (ms)', fontsize=11)
ax1.set_title('Execution Time: Local vs Multi-VM', fontsize=12, fontweight='bold')
ax1.set_xticks(x)
ax1.set_xticklabels([f'{s//1000}K' for s in graph_sizes])
ax1.legend(fontsize=9)
ax1.grid(True, alpha=0.3, axis='y')
ax1.set_yscale('log')

# ===== Plot 2: Local Speedup Analysis =====
ax2 = fig.add_subplot(gs[1, 0])
x = np.arange(len(graph_sizes))
width = 0.35

speedups_2p = []
speedups_4p = []
for size in graph_sizes:
    seq_time = df[(df['Implementation'] == 'Sequential') & 
                  (df['Graph_Size'] == size)]['Time_ms'].values[0]
    time_2p = df[(df['Implementation'] == 'Distributed-Local') & 
                 (df['Graph_Size'] == size) & 
                 (df['Processes'] == 2)]['Time_ms'].values[0]
    time_4p = df[(df['Implementation'] == 'Distributed-Local') & 
                 (df['Graph_Size'] == size) & 
                 (df['Processes'] == 4)]['Time_ms'].values[0]
    speedups_2p.append(seq_time / time_2p if time_2p > 0 else 0)
    speedups_4p.append(seq_time / time_4p if time_4p > 0 else 0)

ax2.bar(x - width/2, speedups_2p, width, label='2 Processes', color=colors['Local-2'])
ax2.bar(x + width/2, speedups_4p, width, label='4 Processes', color=colors['Local-4'])
ax2.axhline(y=1, color='gray', linestyle='--', alpha=0.5, label='Baseline')
ax2.set_xlabel('Graph Size', fontsize=10)
ax2.set_ylabel('Speedup Factor', fontsize=10)
ax2.set_title('Local Execution Speedup', fontsize=11, fontweight='bold')
ax2.set_xticks(x)
ax2.set_xticklabels([f'{s//1000}K' for s in graph_sizes])
ax2.legend(fontsize=8)
ax2.grid(True, alpha=0.3, axis='y')

# ===== Plot 3: Multi-VM Overhead =====
ax3 = fig.add_subplot(gs[1, 1])
x = np.arange(len(graph_sizes))
width = 0.35

overhead_6p = []
overhead_8p = []
for size in graph_sizes:
    seq_time = df[(df['Implementation'] == 'Sequential') & 
                  (df['Graph_Size'] == size)]['Time_ms'].values[0]
    time_6p = df[(df['Implementation'] == 'Distributed-MultiVM') & 
                 (df['Graph_Size'] == size) & 
                 (df['Processes'] == 6)]['Time_ms'].values[0]
    time_8p = df[(df['Implementation'] == 'Distributed-MultiVM') & 
                 (df['Graph_Size'] == size) & 
                 (df['Processes'] == 8)]['Time_ms'].values[0]
    overhead_6p.append(time_6p / seq_time if seq_time > 0 else 0)
    overhead_8p.append(time_8p / seq_time if seq_time > 0 else 0)

ax3.bar(x - width/2, overhead_6p, width, label='6 Processes', color=colors['MultiVM-6'])
ax3.bar(x + width/2, overhead_8p, width, label='8 Processes', color=colors['MultiVM-8'])
ax3.axhline(y=1, color='gray', linestyle='--', alpha=0.5, label='Sequential Baseline')
ax3.set_xlabel('Graph Size', fontsize=10)
ax3.set_ylabel('Slowdown Factor', fontsize=10)
ax3.set_title('Multi-VM Communication Overhead', fontsize=11, fontweight='bold')
ax3.set_xticks(x)
ax3.set_xticklabels([f'{s//1000}K' for s in graph_sizes])
ax3.legend(fontsize=8)
ax3.grid(True, alpha=0.3, axis='y')
ax3.set_yscale('log')

# ===== Plot 4: Parallel Efficiency =====
ax4 = fig.add_subplot(gs[1, 2])
x = np.arange(len(graph_sizes))
width = 0.2

for i, (np_val, label, color) in enumerate([(2, '2P Local', 'Local-2'), 
                                              (4, '4P Local', 'Local-4'),
                                              (6, '6P Multi-VM', 'MultiVM-6'),
                                              (8, '8P Multi-VM', 'MultiVM-8')]):
    efficiencies = []
    for size in graph_sizes:
        seq_time = df[(df['Implementation'] == 'Sequential') & 
                      (df['Graph_Size'] == size)]['Time_ms'].values[0]
        
        if np_val <= 4:
            dist_time = df[(df['Implementation'] == 'Distributed-Local') & 
                          (df['Graph_Size'] == size) & 
                          (df['Processes'] == np_val)]['Time_ms'].values[0]
        else:
            dist_time = df[(df['Implementation'] == 'Distributed-MultiVM') & 
                          (df['Graph_Size'] == size) & 
                          (df['Processes'] == np_val)]['Time_ms'].values[0]
        
        speedup = seq_time / dist_time if dist_time > 0 else 0
        efficiency = (speedup / np_val) * 100
        efficiencies.append(efficiency)
    
    ax4.bar(x + (i-1.5)*width, efficiencies, width, label=label, color=colors[color])

ax4.axhline(y=100, color='gray', linestyle='--', alpha=0.5, label='100% Efficient')
ax4.set_xlabel('Graph Size', fontsize=10)
ax4.set_ylabel('Efficiency (%)', fontsize=10)
ax4.set_title('Parallel Efficiency Comparison', fontsize=11, fontweight='bold')
ax4.set_xticks(x)
ax4.set_xticklabels([f'{s//1000}K' for s in graph_sizes])
ax4.legend(fontsize=7)
ax4.grid(True, alpha=0.3, axis='y')

# ===== Plot 5-7: Iteration Counts =====
for idx, size in enumerate([10000, 20000, 30000]):
    ax = fig.add_subplot(gs[2, idx])
    
    df_size = df[df['Graph_Size'] == size]
    
    configs = ['Seq', 'L-2P', 'L-4P', 'MV-6P', 'MV-8P']
    iterations = []
    
    # Sequential (no iterations tracked)
    iterations.append(0)
    
    # Local 2P
    iter_val = df_size[(df_size['Implementation'] == 'Distributed-Local') & 
                       (df_size['Processes'] == 2)]['Iterations'].values
    iterations.append(int(iter_val[0]) if len(iter_val) > 0 else 0)
    
    # Local 4P
    iter_val = df_size[(df_size['Implementation'] == 'Distributed-Local') & 
                       (df_size['Processes'] == 4)]['Iterations'].values
    iterations.append(int(iter_val[0]) if len(iter_val) > 0 else 0)
    
    # Multi-VM 6P
    iter_val = df_size[(df_size['Implementation'] == 'Distributed-MultiVM') & 
                       (df_size['Processes'] == 6)]['Iterations'].values
    iterations.append(int(iter_val[0]) if len(iter_val) > 0 else 0)
    
    # Multi-VM 8P
    iter_val = df_size[(df_size['Implementation'] == 'Distributed-MultiVM') & 
                       (df_size['Processes'] == 8)]['Iterations'].values
    iterations.append(int(iter_val[0]) if len(iter_val) > 0 else 0)
    
    bars = ax.bar(configs, iterations, color=['#2E86AB', '#A23B72', '#F18F01', '#06A77D', '#C73E1D'])
    ax.set_title(f'{size//1000}K Nodes - Iterations', fontsize=11, fontweight='bold')
    ax.set_ylabel('Iterations', fontsize=10)
    ax.grid(True, alpha=0.3, axis='y')
    
    # Add value labels on bars
    for bar in bars:
        height = bar.get_height()
        if height > 0:
            ax.text(bar.get_x() + bar.get_width()/2., height,
                   f'{int(height)}', ha='center', va='bottom', fontsize=9)

plt.savefig('performance_analysis.png', dpi=300, bbox_inches='tight')
print("Visualization saved as 'performance_analysis.png'\n")
plt.show()
