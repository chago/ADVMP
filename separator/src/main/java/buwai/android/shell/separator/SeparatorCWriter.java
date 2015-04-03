package buwai.android.shell.separator;

import buwai.android.dexlib2.helper.MethodHelper;
import org.jf.dexlib2.iface.Method;

import java.io.*;
import java.util.*;

/**
 * Created by buwai on 2015/4/2.
 */
public class SeparatorCWriter {

    private File mOutFile;

    private List<Method> mSeparatedMethod;

    private Map<String, List<Method>> classes = new HashMap<>();

    private List<String> registerNativesNames = new ArrayList<>();

    private int registerNativesIndex = 0;

    public SeparatorCWriter (File outFile, List<Method> separatedMethod) {
        mOutFile = outFile;
        mSeparatedMethod = separatedMethod;
    }

    public void write() throws IOException {
        try (BufferedWriter fileWriter = new BufferedWriter(new FileWriter(mOutFile))) {
            int index = 0;
            for (Method method : mSeparatedMethod) {
                String definingClass = method.getDefiningClass();
                if (classes.containsKey(definingClass)) {
                    classes.get(definingClass).add(method);
                } else {
                    List<Method> ms = new ArrayList<>();
                    ms.add(method);
                    classes.put(definingClass, ms);
                }

                writeMethod(index, method, fileWriter);
                index++;
            }

            write_registerNatives(fileWriter);

            fileWriter.write("void registerFunctions(JNIEnv* env) {");
            fileWriter.newLine();
            for (String registerNativesName : registerNativesNames) {
                fileWriter.write(String.format("if (!%s(env)) { MY_LOG_ERROR(\"register method fail.\"); return; }", registerNativesName));
                fileWriter.newLine();
            }
            fileWriter.newLine();
            fileWriter.write("}");
            fileWriter.newLine();
        }
    }

    private void writeMethod(int index, Method method, BufferedWriter fileWriter) throws IOException {
        StringBuffer sb = new StringBuffer();
        sb.append(MethodHelper.genTypeInNative(method));
        sb.append(" ");
        sb.append(method.getName());
        sb.append(" (");
        sb.append(MethodHelper.genParamTypeListInNative(method));
        sb.append(") {");
        fileWriter.write(sb.toString());
        fileWriter.newLine();

        sb.delete(0, sb.length());
        sb.append("jvalue result = BWdvmInterpretPortable(gAdvmp.ycFile->GetSeparatorData(");
        sb.append(index);
        sb.append("), env, thiz");

        List<? extends CharSequence> params = method.getParameterTypes();
        for (int i = 0; i < params.size(); i++) {
            sb.append(", ");
            sb.append(MethodHelper.paramNames[i]);
        }
        sb.append(");");
        fileWriter.write(sb.toString());
        fileWriter.newLine();

        sb.delete(0, sb.length());
        sb.append("return ");
        char cType = method.getReturnType().charAt(0);
        switch (cType) {
            case 'Z':
                sb.append("result.z");
                break;
            case 'B':
                sb.append("result.b");
                break;
            case 'S':
                sb.append("result.s");
                break;
            case 'C':
                sb.append("result.c");
                break;
            case 'I':
                sb.append("result.i");
                break;
            case 'J':
                sb.append("result.j");
                break;
            case 'F':
                sb.append("result.f");
                break;
            case 'D':
                sb.append("result.d");
                break;
            case 'L':
                sb.append("result.l");
                break;
            case '[':
                sb.append("result.l");
                break;
        }
        sb.append(";}");
        fileWriter.write(sb.toString());
        fileWriter.newLine();
    }

    private void write_registerNatives(BufferedWriter fileWriter) throws IOException {
        for (Map.Entry<String, List<Method>> entry : classes.entrySet()) {
            fileWriter.write(String.format("bool registerNatives%d(JNIEnv* env) {", registerNativesIndex));
            registerNativesNames.add("registerNatives" + registerNativesIndex);
            registerNativesIndex++;
            fileWriter.newLine();

            String type = entry.getKey();
            type = type.substring(1, type.lastIndexOf(';'));
            fileWriter.write(String.format("const char* classDesc = \"%s\";", type));
            fileWriter.newLine();

            fileWriter.write("const JNINativeMethod methods[] = {");
            fileWriter.newLine();
            for (Method method : entry.getValue()) {
                fileWriter.write(MethodHelper.genJNINativeMethod(method));
                fileWriter.newLine();
            }
            fileWriter.write("};");
            fileWriter.newLine();
        }
        fileWriter.write("jclass clazz = env->FindClass(classDesc);");
        fileWriter.newLine();

        fileWriter.write(String.format("if (!clazz) { MY_LOG_ERROR(\"can't find class:%%s!\", classDesc); return false; }"));
        fileWriter.newLine();

        fileWriter.write("bool bRet = false;");
        fileWriter.newLine();

        fileWriter.write("if ( JNI_OK == env->RegisterNatives(clazz, methods, array_size(methods)) ) { bRet = true; } ");
        fileWriter.newLine();
        fileWriter.write("else { MY_LOG_ERROR(\"classDesc:%s, register method fail.\", classDesc); }");
        fileWriter.newLine();

        fileWriter.write("env->DeleteLocalRef(clazz); return bRet; }");
        fileWriter.newLine();
    }

}
