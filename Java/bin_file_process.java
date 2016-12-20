package bin_file_process;

import java.io.*;

public class bin_file_process {
    public static void main(String[] args) {
        System.out.println("demo");
        try {
            FileInputStream fin = new FileInputStream("0x10800000");
            DataInputStream data = new DataInputStream(fin);
            StringBuilder hexstream = new StringBuilder();
            int value;
            while((value = data.read()) != -1) {
                hexstream.append(String.format("%02X ", value));
            }
            System.out.println(hexstream.toString());
            fin.close();
            data.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
