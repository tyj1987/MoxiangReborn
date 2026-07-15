# Moxian-Reborn Server Dockerfile
# Multi-stage build for production deployment
#
# Build: docker build -t moxian-server .
# Run:   docker run -p 6001:6001 -p 7001:7001 -p 8001:8001 moxian-server

# ============================================================================
# Stage 1: Build
# ============================================================================
FROM mcr.microsoft.com/windows/servercore:ltsc2022 AS builder

# Install Visual Studio Build Tools
RUN powershell -Command \
    Invoke-WebRequest -Uri https://aka.ms/vs/17/release/vs_buildtools.exe -OutFile vs_buildtools.exe; \
    Start-Process -FilePath vs_buildtools.exe -ArgumentList '--quiet', '--norestart', '--nocache', \
        '--add', 'Microsoft.VisualStudio.Workload.VCTools', \
        '--add', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', \
        '--add', 'Microsoft.VisualStudio.Component.Windows11SDK.22621' -Wait; \
    Remove-Item vs_buildtools.exe

# Install CMake
RUN powershell -Command \
    Invoke-WebRequest -Uri https://github.com/Kitware/CMake/releases/download/v3.28.1/cmake-3.28.1-windows-x86_64.msi -OutFile cmake.msi; \
    Start-Process msiexec.exe -ArgumentList '/i', 'cmake.msi', '/quiet', 'ADD_CMAKE_TO_PATH=System' -Wait; \
    Remove-Item cmake.msi

WORKDIR /app

# Copy source code
COPY modern/ ./modern/
COPY MODERNIZATION_PLAN.md ./

# Configure and build
RUN cmake -S modern -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build --config Release --parallel

# ============================================================================
# Stage 2: Runtime
# ============================================================================
FROM mcr.microsoft.com/windows/servercore:ltsc2022 AS runtime

WORKDIR /app

# Copy built binaries
COPY --from=builder /app/build/tools/MoxianLoginServer/Release/ ./
COPY --from=builder /app/build/tools/MoxianAgentServer/Release/ ./

# Copy configuration files
COPY docker/ ./docker/

# Expose ports
# Distribute (Login): 6001
# Agent: 7001
# Map: 8001-8010
EXPOSE 6001 7001 8001-8010

# Health check
HEALTHCHECK --interval=30s --timeout=10s --retries=3 \
    CMD powershell -Command "Test-NetConnection -ComputerName localhost -Port 6001 | Select-Object -ExpandProperty TcpTestSucceeded"

# Default command
CMD ["powershell", "-Command", "Start-Process -FilePath '.\\mxh_login_server.exe' -NoNewWindow; Start-Process -FilePath '.\\mxh_agent_server_CHINA.exe' -NoNewWindow; Wait-Process -Name 'mxh_login_server','mxh_agent_server_CHINA'"]
