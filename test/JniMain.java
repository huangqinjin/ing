public class JniMain {
	public static void main(String[] args) {
        System.loadLibrary(args[0]);
        jmain(args);
    }

    private static native int jmain(String... args);
}
