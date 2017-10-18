# Tugas Besar 1 IF3130 Jaringan Komputer

## Sliding Window

### Petunjuk Penggunaan Program

Untuk melakukan kompilasi, hanya perlu pergi ke direktori tempat project diletakkan. Setelah itu, untuk memulai kompilasi perintah **make** atau **make all** dimasukkan pada terminal.

Untuk menjalankan program, diperlukan 2 buah window terminal. Window yang pertama akan digunakan sebagai sender, sedangkan window yang kedua akan dipakai sebagai receiver.

#### Program sender

Program sender dijalankan dengan cara memasukkan 5 buah argumen tambahan, yaitu dengan perintah:
...
./sendfile <filename> <windowsize> <buffersize> <destination_ip> <destination_port>
...

| Argumen | Keterangan |
|--------------|-----------------|
| filename | Argumen untuk menyatakan nama file yang akan dikirim |
| windowsize | Ukuran window dari sender (SWS) |
| buffersize | Fixed dan tidak bergantung dari argumen, pasti sebesar 256 byte |
| destination_ip | Alamat IP tempat paket akan dikirimkan |
| destination_port | Port dimana paket dikirimkan pada alamat IP yang terdefinisi |

#### Program receiver

Program receiver dijalankan dengan cara memasukkan 4 buah argumen tambahan, yaitu dengan memasukkan perintah:
...
./recvfile <filename> <windowsize> <buffersize> <port>
...

| Argumen | Keterangan |
|--------------|-----------------|
| filename | Nama file yang akan dibuat setelah pengiriman paket selesai |
| windowsize | Ukuran window pada sisi receiver (RWS) |
| buffersize | Fixed dan tidak bergantung dari argumen, pasti sebesar 256 byte |
| port | Port yang dibentuk oleh receiver untuk mendapatkan paket dari sender |

### Cara Kerja Sliding Window
1. Pertama-tama, file eksternal yang ingin dikirim dimuat terlebih dahulu ke buffer yang ada pada program sender kemudian dibungkus menjadi frame sebesar window size (SWS) yang dimasukkan sesuai dengan keinginan user dari argumen program.
2. Setelah itu, frame yang sudah dibuat dikirimkan saat menjalankan program (dengan memberi argumen).
3. Jika sebuah frame sampai pada receiver, maka receiver akan membalas dengan mengirimkan ACK[<next-frame>], kecuali terdapat kegagalan pada frame sebelumnya. Jika terjadi kegagalan, ACK yang dikirim tetap ACK[<next-frame>] dari frame yang terakhir berhasil diterima.
4. Pengiriman selanjutnya oleh pihak sender dilakukan sebanyak SWS byte lagi yang dimulai dari ACK yang diterima.
5. Jika kegagalan terjadi pada sebuah pengiriman (timeout atau packet loss), maka pengiriman tersebut akan diulangi kembali berdasarkan ACK terakhir yang sudah diterima sebelum pengiriman tersebut pada sisi sender. Dengan demikian, sender akan mengirimkan kembali paket/frame sebanyak SWS byte yang sesuai dengan sequence ACK-nya.

### Pembagian Tugas
 
* 13515009 - Gisela Supardi: bungkus paket, bungkus ACK
* 13515033 - Andika Kusuma: implementasi sliding window, membuat socket
* 13515066 - Ferdinandus Richard: hitung checksum, check checksum, implementasi log

### Pertanyaan dan Jawaban

Apa yang terjadi jika advertised window yang dikirim bernilai 0? Apa cara untuk menangani hal tersebut?

Advertised window dari sebuah sender atau receiver tidak boleh bernilai 0. Ini karena jika nilainya 0, berarti sebuah sender tidak dapat mengirimkan apapun, karena window sizenya berukuran 0. Selain itu, jika receivernya yang memiliki advertised window sebesar 0, berarti receiver tidak dapat menerima apapun dari sender karena window size-nya 0.

Sebutkan field data yang terdapat TCP Header serta ukurannya, ilustrasikan, dan jelaskan kegunaan dari masing-masing field data tersebut!

| Field Data | Ukuran | Kegunaan |
|---------------|-----------------|-----------------|
| Source TCP port | 2 byte | Menyatakan port asal paket dikirim |
| Destination TCP port | 2 byte | Menyatakan port tujuan paket akan dikirim |
| Sequence number | 4 byte | Menyatakan urutan paket ke-berapakah paket tersebut |
| Acknowledge number / ACK | 4 byte | Menyatakan sequence number yang paling terakhir dikirim atau paling terakhir diharapkan terkirim |
| Data offset | 4 bit | Menyatakan offset data yang merupakan pembagian ukuran paket dengan 4 byte |
| Reserved byte | 3 bit | Hanya bertujuan untuk alignment agar data kelipatan 4 bit |
| Control flags | 9 bit | Menyatakan flag-flag tertentu seperti SYN (untuk melakukan koneksi), ACK (mengakui penerimaan data), dan sebagainya |
| WIndow size | 2 byte | Menyatakan banyaknya data yang dikirim oleh sender kepada receiver sebelum sender menerima ACK dari receiver |
| Checksum | 2 byte | Menyatakan sebuah nilai checksum yang berguna untuk pendeteksian error pada paket/message yang dikirim |
| Urgent pointer | 2 byte | Seringkali diabaikan dan diisi dengan bit-bit 0 |
| Optional data | 0 - 40 byte | Tidak harus digunakan, dapat berisi ACK khusus atau algoritma window scaling |

### About Team

#### Blackjack

13515009 Gisela Supardi

13515033 Andika Kusuma

13515066 Ferdinandus Richard


