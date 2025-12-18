from Crypto.Cipher import Blowfish
from Crypto.Random import get_random_bytes
from Crypto.Util.Padding import (
    pad,
    unpad,
)


def run_blowfish():
    print("=== Blowfish demo ===")
    key = get_random_bytes(16)  # 4..56 bytes
    data = b"Message de test Blowfish"

    bs = Blowfish.block_size  # 8 octets
    cipher = Blowfish.new(key, Blowfish.MODE_CBC)
    ciphertext = cipher.encrypt(pad(data, bs))

    cipher_dec = Blowfish.new(key, Blowfish.MODE_CBC, iv=cipher.iv)
    plaintext = unpad(cipher_dec.decrypt(ciphertext), bs)

    print(f"Plaintext : {data}")
    print(f"Ciphertext (hex) : {ciphertext.hex()}")
    print(f"Decrypted : {plaintext}")


if __name__ == "__main__":
    run_blowfish()
