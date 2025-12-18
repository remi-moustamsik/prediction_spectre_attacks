from Crypto.Cipher import AES
from Crypto.Random import get_random_bytes


def run_aes():
    print("=== AES demo ===")
    key = get_random_bytes(16)  # AES-128
    data = b"Message de test AES"

    cipher = AES.new(key, AES.MODE_EAX)
    ciphertext, tag = cipher.encrypt_and_digest(data)

    cipher_dec = AES.new(key, AES.MODE_EAX, nonce=cipher.nonce)
    plaintext = cipher_dec.decrypt(ciphertext)
    cipher_dec.verify(tag)

    print(f"Plaintext : {data}")
    print(f"Ciphertext (hex) : {ciphertext.hex()}")
    print(f"Decrypted : {plaintext}")


if __name__ == "__main__":
    run_aes()
