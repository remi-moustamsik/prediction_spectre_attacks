from Crypto.Cipher import ARC4
from Crypto.Random import get_random_bytes


def run_arc():
    print("=== ARC4 demo ===")
    key = get_random_bytes(16)
    data = b"Message de test ARC4"

    cipher = ARC4.new(key)
    ciphertext = cipher.encrypt(data)

    cipher_dec = ARC4.new(key)
    plaintext = cipher_dec.encrypt(ciphertext)

    print(f"Plaintext : {data}")
    print(f"Ciphertext (hex) : {ciphertext.hex()}")
    print(f"Decrypted : {plaintext}")


if __name__ == "__main__":
    run_arc()
