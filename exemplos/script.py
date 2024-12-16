import os

# Função para gerar arquivos com nome baseado em uma string de 1 até 2^16 caracteres
def generate_file(file_name, size_in_bytes):
    with open(file_name, "wb") as f:
        # Preencher o arquivo com o caractere 'A' repetido até o tamanho especificado
        f.write(b"A" * size_in_bytes)  # Usando o caractere 'A' repetido para preencher o arquivo
    print(f"Arquivo {file_name} gerado com tamanho {size_in_bytes} bytes.")

# Gerar arquivos com nomes de 1 a 2^16 caracteres
for i in range(1, 2**16 + 1):
    # Gerar nome do arquivo como uma string de 'i' caracteres, todos 'a'
    file_name = "a" * i + ".txt"
    generate_file(file_name, i)  # O tamanho do arquivo será 'i' bytes
