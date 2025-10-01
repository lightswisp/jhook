public class test {
    public static void main(String[] args) {
        while (true) {
            System.out.println("Hello");
            try {
                Thread.sleep(1000); 
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }
}

