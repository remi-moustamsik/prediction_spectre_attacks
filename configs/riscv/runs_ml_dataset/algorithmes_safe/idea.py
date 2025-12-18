from Crypto.Cipher import IDEA
from Crypto.Random import get_random_bytes
from Crypto.Util.Padding import (
    pad,
    unpad,
)


def run_idea():
    print("=== IDEA demo ===")
    key = get_random_bytes(16)  # 128 bits
    data = b"Message de test IDEA"

    bs = IDEA.block_size  # 8 octets
    iv = get_random_bytes(bs)
    cipher = IDEA.new(key, IDEA.MODE_CBC, iv=iv)
    ciphertext = cipher.encrypt(pad(data, bs))

    cipher_dec = IDEA.new(key, IDEA.MODE_CBC, iv=iv)
    plaintext = unpad(cipher_dec.decrypt(ciphertext), bs)

    print(f"Plaintext : {data}")
    print(f"Ciphertext (hex) : {ciphertext.hex()}")
    print(f"Decrypted : {plaintext}")


if __name__ == "__main__":
    run_idea()
