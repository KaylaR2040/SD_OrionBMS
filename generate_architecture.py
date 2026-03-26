#!/usr/bin/env python3
"""
Auto-generate Mermaid architecture diagram from STM32 codebase
Usage: python3 generate_architecture.py
"""

import os
import re
from pathlib import Path
from collections import defaultdict

class CodebaseAnalyzer:
    def __init__(self, root_dir):
        self.root = Path(root_dir)
        self.files = {'src': [], 'include': [], 'docs': []}
        self.includes = defaultdict(set)
        self.functions = defaultdict(list)
        self.function_calls = defaultdict(set)
        self.globals = defaultdict(list)
        
    def scan_files(self):
        """Scan all C/H files"""
        for folder in ['src', 'include']:
            path = self.root / folder
            if path.exists():
                for file in list(path.glob('*.c')) + list(path.glob('*.h')):
                    self.files[folder].append(file)
                    
    def parse_includes(self, filepath):
        """Extract #include statements"""
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                content = f.read()
                includes = re.findall(r'#include\s+"([^"]+)"', content)
                return set(includes)
        except:
            return set()
            
    def parse_functions(self, filepath):
        """Extract function definitions"""
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                content = f.read()
                # Remove comments
                content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
                content = re.sub(r'//.*?\n', '\n', content)
                # Find functions
                funcs = re.findall(r'\n(\w+\s+\**\s*\w+)\s*\([^)]*\)\s*\{', content)
                return [f.strip().split()[-1] for f in funcs]
        except:
            return []
            
    def parse_function_calls(self, filepath):
        """Extract function calls"""
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                content = f.read()
                # Remove comments
                content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
                content = re.sub(r'//.*?\n', '\n', content)
                # Find function calls
                calls = re.findall(r'(\w+)\s*\(', content)
                return set(calls)
        except:
            return set()
            
    def parse_globals(self, filepath):
        """Extract global variables"""
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                content = f.read()
                # Look for globals like g_*, UART_*, ADC_*, etc
                globs = re.findall(r'\n((?:extern\s+)?[\w\s\*]+\s+[g_]\w+)\s*[;=]', content)
                globs += re.findall(r'\n((?:UART|ADC|SPI|CAN|FDCAN)\w*HandleTypeDef\s+\w+)\s*[;=]', content)
                return [g.strip() for g in globs]
        except:
            return []
    
    def analyze(self):
        """Run full analysis"""
        self.scan_files()
        
        # Parse all files
        for folder in ['src', 'include']:
            for file in self.files[folder]:
                filename = file.name
                self.includes[filename] = self.parse_includes(file)
                self.functions[filename] = self.parse_functions(file)
                self.function_calls[filename] = self.parse_function_calls(file)
                self.globals[filename] = self.parse_globals(file)
                
    def categorize_file(self, filename):
        """Determine subsystem category"""
        name = filename.lower()
        if 'can' in name and 'message' in name:
            return 'can_msg'
        elif 'can' in name:
            return 'can'
        elif 'spi' in name or 'max' in name or 'asci' in name:
            return 'spi'
        elif 'adc' in name:
            return 'adc'
        elif 'led' in name:
            return 'led'
        elif 'log' in name or 'uart' in name:
            return 'log'
        elif 'error' in name:
            return 'error'
        elif 'init' in name or 'clock' in name:
            return 'init'
        elif 'main' in name:
            return 'main'
        else:
            return 'other'
            
    def generate_mermaid(self, output_file):
        """Generate detailed Mermaid flowchart with function-level details"""
        lines = [
            "```mermaid",
            "flowchart TB",
            ""
        ]
        
        # Track all nodes
        node_ids = {}
        node_counter = 0
        
        # Group files by category
        categories = defaultdict(list)
        for folder in ['src', 'include']:
            for file in self.files[folder]:
                cat = self.categorize_file(file.name)
                categories[cat].append(file.name)
        
        # Combine related categories
        combined = {
            'MAIN': ('🚀 MAIN', ['main']),
            'INIT': ('🔧 INIT', ['init']),
            'CAN_SYSTEM': ('� CAN SYSTEM', ['can', 'can_msg']),
            'SPI_SYSTEM': ('🔵 SPI SYSTEM', ['spi']),
            'ADC_SYSTEM': ('📊 ADC SYSTEM', ['adc']),
            'LED_SYSTEM': ('🟡 LED', ['led']),
            'LOG_SYSTEM': ('🟣 LOGGING', ['log']),
            'ERROR': ('🔴 ERROR', ['error']),
            'HAL': ('⚙️ HAL/SYSTEM', ['other'])
        }
        
        # Create detailed subgraphs
        for group_name, (label, cats) in combined.items():
            files_in_group = []
            for cat in cats:
                files_in_group.extend(categories.get(cat, []))
            
            if not files_in_group:
                continue
                
            lines.append(f"    subgraph {group_name}[{label}]")
            lines.append("        direction TB")
            
            for file in sorted(files_in_group):
                node_id = f"N{node_counter}"
                node_counter += 1
                node_ids[file] = node_id
                
                funcs = self.functions.get(file, [])
                
                # Show file with key functions
                if file.endswith('.c') and funcs:
                    # Show top functions
                    func_list = "<br/>".join(f"• {f}()" for f in funcs[:5])
                    if len(funcs) > 5:
                        func_list += f"<br/>• +{len(funcs)-5} more"
                    lines.append(f"        {node_id}[\"<b>{file}</b><br/>{func_list}\"]")
                elif file.endswith('.h'):
                    lines.append(f"        {node_id}[\"<b>{file}</b><br/>Interface\"]")
                else:
                    lines.append(f"        {node_id}[\"{file}\"]")
            
            lines.append("    end")
            lines.append("")
        
        lines.append("    %% ========== MAIN FLOW ==========")
        
        # Show main initialization sequence
        if 'main.c' in node_ids:
            main_id = node_ids['main.c']
            
            if 'init.c' in node_ids:
                lines.append(f"    {main_id} ===>|\"1. System_AppInit()\"| {node_ids['init.c']}")
            if 'adc.c' in node_ids:
                lines.append(f"    {main_id} ===>|\"2. ADC_App_Init()\"| {node_ids['adc.c']}")
            if 'spi.c' in node_ids:
                lines.append(f"    {main_id} ===>|\"3. SPI_App_Init()\"| {node_ids['spi.c']}")
            if 'can.c' in node_ids:
                lines.append(f"    {main_id} ===>|\"4. CAN_App_Init()\"| {node_ids['can.c']}")
            if 'led.c' in node_ids:
                lines.append(f"    {main_id} ===>|\"5. LED_All_Pulse()\"| {node_ids['led.c']}")
        
        lines.append("")
        lines.append("    %% ========== SERVICE TASKS ==========")
        
        # Show periodic service task calls
        if 'main.c' in node_ids:
            main_id = node_ids['main.c']
            
            if 'adc.c' in node_ids:
                lines.append(f"    {main_id} -->|\"Loop: ADC_ServiceTask()\"| {node_ids['adc.c']}")
            if 'spi.c' in node_ids:
                lines.append(f"    {main_id} -->|\"Loop: SPI_ServiceTask()\"| {node_ids['spi.c']}")
            if 'can.c' in node_ids:
                lines.append(f"    {main_id} -->|\"Loop: CAN_ServiceTask()\"| {node_ids['can.c']}")
        
        lines.append("")
        lines.append("    %% ========== CAN MESSAGE FLOW ==========")
        
        # CAN system internal flow
        if 'can.c' in node_ids and 'can_messages.c' in node_ids:
            lines.append(f"    {node_ids['can.c']} -->|\"Encode thermistor data<br/>CAN_Msg_EncodeThermJ1939Claim()\"| {node_ids['can_messages.c']}")
            lines.append(f"    {node_ids['can.c']} -->|\"Encode BMS broadcast<br/>CAN_Msg_EncodeThermBMSBroadcast()\"| {node_ids['can_messages.c']}")
            lines.append(f"    {node_ids['can.c']} -->|\"Encode cell voltage<br/>CAN_Msg_EncodeL180CellVoltage()\"| {node_ids['can_messages.c']}")
            lines.append(f"    {node_ids['can_messages.c']} -.->|\"Returns payload + ID\"| {node_ids['can.c']}")
            
        if 'can.c' in node_ids and 'can.h' in node_ids:
            lines.append(f"    {node_ids['can.h']} -.->|\"Declares API\"| {node_ids['can.c']}")
        if 'can_messages.h' in node_ids and 'can_messages.c' in node_ids:
            lines.append(f"    {node_ids['can_messages.h']} -.->|\"Defines structs\"| {node_ids['can_messages.c']}")
        
        lines.append("")
        lines.append("    %% ========== SPI/MAX17841B FLOW ==========")
        
        # SPI system internal flow
        if 'spi.c' in node_ids:
            spi_id = node_ids['spi.c']
            
            if 'spi_max.h' in node_ids:
                lines.append(f"    {spi_id} -->|\"SPI_Max_RequestCellFrame()<br/>Read battery cell voltages\"| {spi_id}")
            if 'max17841b.h' in node_ids:
                lines.append(f"    {node_ids['max17841b.h']} -.->|\"Register definitions<br/>RD_NXT_MSG, RD_RX_STATUS\"| {spi_id}")
            
            lines.append(f"    {spi_id} -->|\"ASCI_ReadNextMsg()<br/>Poll MAX17841B\"| {spi_id}")
        
        lines.append("")
        lines.append("    %% ========== CROSS-MODULE DEPENDENCIES ==========")
        
        # Logging usage
        if 'log.c' in node_ids:
            log_id = node_ids['log.c']
            for file in ['adc.c', 'spi.c', 'can.c', 'init.c']:
                if file in node_ids:
                    lines.append(f"    {node_ids[file]} -.->|\"LOG_INFO()\"| {log_id}")
        
        # LED usage
        if 'led.c' in node_ids:
            led_id = node_ids['led.c']
            for file in ['adc.c', 'spi.c', 'can.c']:
                if file in node_ids:
                    lines.append(f"    {node_ids[file]} -.->|\"LED control\"| {led_id}")
        
        # Error handling
        if 'error_handling.c' in node_ids:
            err_id = node_ids['error_handling.c']
            for file in ['adc.c', 'spi.c', 'can.c', 'init.c', 'log.c']:
                if file in node_ids:
                    lines.append(f"    {node_ids[file]} -.->|\"Error_Handler()\"| {err_id}")
        
        lines.append("")
        lines.append("    %% ========== TIMING INFO ==========")
        
        # Add timing info nodes
        lines.append("    TIMING[\"⏱️ TIMING<br/>ADC: 1s delay, 5s period<br/>SPI: 2s delay, 5s period<br/>CAN: 3s delay, 5s period\"]")
        lines.append("    style TIMING fill:#fff9c4,stroke:#f57f17,stroke-width:2px")
        
        lines.append("")
        lines.append("    %% ========== STYLING ==========")
        
        # Apply styles based on groups
        style_map = {
            'MAIN': 'mainStyle',
            'INIT': 'initStyle', 
            'CAN_SYSTEM': 'canStyle',
            'SPI_SYSTEM': 'spiStyle',
            'ADC_SYSTEM': 'adcStyle',
            'LED_SYSTEM': 'ledStyle',
            'LOG_SYSTEM': 'logStyle',
            'ERROR': 'errStyle',
            'HAL': 'otherStyle'
        }
        
        lines.append("    classDef mainStyle fill:#e3f2fd,stroke:#1976d2,stroke-width:4px")
        lines.append("    classDef initStyle fill:#f3e5f5,stroke:#7b1fa2,stroke-width:2px")
        lines.append("    classDef canStyle fill:#e8f5e9,stroke:#388e3c,stroke-width:3px")
        lines.append("    classDef spiStyle fill:#e1f5fe,stroke:#0277bd,stroke-width:3px")
        lines.append("    classDef adcStyle fill:#fff3e0,stroke:#f57c00,stroke-width:3px")
        lines.append("    classDef ledStyle fill:#fffde7,stroke:#f9a825,stroke-width:2px")
        lines.append("    classDef logStyle fill:#f3e5f5,stroke:#8e24aa,stroke-width:2px")
        lines.append("    classDef errStyle fill:#ffebee,stroke:#c62828,stroke-width:3px")
        lines.append("    classDef otherStyle fill:#eceff1,stroke:#546e7a,stroke-width:2px")
        
        lines.append("")
        
        # Apply class to nodes
        for group_name, (label, cats) in combined.items():
            files_in_group = []
            for cat in cats:
                files_in_group.extend(categories.get(cat, []))
            
            node_list = ",".join(node_ids[f] for f in files_in_group if f in node_ids)
            if node_list and group_name in style_map:
                lines.append(f"    class {node_list} {style_map[group_name]}")
        
        lines.append("```")
        
        # Write to file
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write('\n'.join(lines))
        
        print(f"✅ Generated: {output_file}")
        print(f"📊 Files analyzed: {len(node_ids)}")
        print(f"⚡ Functions found: {sum(len(v) for v in self.functions.values())}")
        print(f"🔗 Detailed flow with function calls included")

if __name__ == "__main__":
    # Use current directory or specify path
    codebase_path = os.path.dirname(os.path.abspath(__file__))
    
    print("🔍 Analyzing codebase...")
    analyzer = CodebaseAnalyzer(codebase_path)
    analyzer.analyze()
    
    output_path = os.path.join(codebase_path, "docs", "auto_architecture.md")
    analyzer.generate_mermaid(output_path)
    
    print(f"\n📝 Open in VS Code: {output_path}")
