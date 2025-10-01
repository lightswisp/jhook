public class test {
    public static void main(String[] args) {
        while (true) {
            System.out.println("Hello");
            try {
                Thread.sleep(1000); // wait for 1000 milliseconds (1 second)
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }
}

