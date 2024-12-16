## Ambiente

- Versão do GCC: gcc (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0
- Versão do WSL: 2.3.24.0
- Versão do kernel: 5.15.153.1-2
- Versão do WSLg: 1.0.65
- Versão do MSRDC: 1.2.5620
- Versão do Direct3D: 1.611.1-81528511
- Versão do DXCore: 10.0.26100.1-240331-1435.ge-release
- Versão do Windows: 10.0.19045.5247
- Versão do VScode: 1.96.0

## Executando o programa


### Linux

- Server
```
    gcc -o server ./server.c

    ./server <host ipv4> <porta>
```

- Client
```
    gcc -o client ./client.c

    ./client <host ipv4> <porta> <diretório>
```

*Note: o servidor deverá ser executado antes do cliente.* 
