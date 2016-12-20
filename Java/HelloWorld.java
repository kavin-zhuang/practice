import java.io.*;
import java.util.*;

public class HelloWorld {
    public static void main(String[] args) {
        // test print
        System.out.println("Hello World!");
        // test conversion
        byte[] data = new byte[2];
        data[0] = (byte)0xaa;
        data[1] = (byte)0x55;
        StringBuilder hexstream = new StringBuilder();
        for(byte b : data) {
            hexstream.append(String.format("%02X ", b));
        }
        System.out.println(hexstream.toString());
    }
}
