package buwai.android.shell.base.helper;

import buwai.android.shell.base.TypeDescription;
import org.apache.log4j.Logger;
import pxb.android.axml.*;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Field;

/**
 * Created by Neptunian on 2015/4/1.
 */
public class AndroidManifestHelper {

    private static final Logger log = Logger.getLogger(AndroidManifestHelper.class);

    public static TypeDescription findFirstClass(File manifestFile) throws IOException {
        final String[] className = new String[1];
        AxmlReader reader = new AxmlReader(Util.readFile(manifestFile));

        final String[] packageName = new String[1];

        AxmlWriter writer = new AxmlWriter();

        try { // write non-utf8 string for all platform
            Field fStringItems = AxmlWriter.class.getDeclaredField("stringItems");
            fStringItems.setAccessible(true);
            Object stringItems = fStringItems.get(writer);
            Field fuseUTF8 = stringItems.getClass().getDeclaredField("useUTF8");
            fuseUTF8.setAccessible(true);
            fuseUTF8.setBoolean(stringItems, false);
        } catch (Exception ex) {
            ex.printStackTrace();
        }

        reader.accept(new AxmlVisitor(writer) {
            @Override
            public NodeVisitor child(String ns, String name) {// manifest
                NodeVisitor nv = super.child(ns, name);
                return new NodeVisitor(nv) {

                    @Override
                    public void attr(String ns, String name, int resourceId, int type, Object obj) {
                        if (name.equals("package")) {
                            packageName[0] = (String) obj;
                        }
                        super.attr(ns, name, resourceId, type, obj);
                    }

                    @Override
                    public NodeVisitor child(String ns, String name) {// application
                        if (name.equals("application")) {
                            return new NodeVisitor(super.child(ns, name)) {

                                @Override
                                public void attr(String ns, String name, int resourceId, int type, Object obj) {
                                    if (R.attr.name == resourceId
                                            || ("http://schemas.android.com/apk/res/android".equals(ns) && name
                                            .equals("name"))) {
                                        String applicationName = (String) obj;
                                        if ('.' == applicationName.charAt(0)) {
                                            className[0] = (packageName[0] + applicationName);
                                        } else if (-1 == applicationName.indexOf('.')) {
                                            className[0] = (packageName[0] + "." + applicationName);
                                        } else {
                                            className[0] = (applicationName);
                                        }
                                    }
                                    super.attr(ns, name, resourceId, type, obj);
                                }

                                public NodeVisitor child(String ns, String name) {
                                    if (null == className[0] && "activity".equals(name)) {
                                        return new NodeVisitor(super.child(ns, name)) {

                                            private String classNameStub;

                                            @Override
                                            public void attr(String ns, String name, int resourceId, int type,
                                                             Object obj) {
                                                if (R.attr.name == resourceId
                                                        || ("http://schemas.android.com/apk/res/android".equals(ns) && name
                                                        .equals("name"))) {
                                                    String componentName = (String) obj;
                                                    if ('.' == componentName.charAt(0)) {
                                                        classNameStub = (packageName[0] + componentName);
                                                    } else if (-1 == componentName.indexOf('.')) {
                                                        classNameStub = (packageName[0] + "." + componentName);
                                                    } else {
                                                        classNameStub = (componentName);
                                                    }
                                                }
                                                super.attr(ns, name, resourceId, type, obj);
                                            }

                                            @Override
                                            public NodeVisitor child(String ns, String name) {
                                                if ("intent-filter".equals(name)) {
                                                    return new NodeVisitor() {
                                                        @Override
                                                        public NodeVisitor child(String ns, String name) {
                                                            if ("action".equals(name)) {
                                                                return new NodeVisitor() {
                                                                    @Override
                                                                    public void attr(String ns, String name, int resourceId, int type, Object obj) {
                                                                        if (R.attr.name == resourceId
                                                                                || ("http://schemas.android.com/apk/res/android".equals(ns) && name
                                                                                .equals("name"))) {
                                                                            String componentName = (String) obj;
                                                                            if (componentName.equals("android.intent.action.MAIN")) {
                                                                                className[0] = classNameStub;
                                                                            }
                                                                        }
                                                                    }
                                                                };
                                                            }
                                                            return super.child(ns, name);
                                                        }
                                                    };
                                                }
                                                return super.child(ns, name);
                                            }
                                        };
                                    }
                                    return super.child(ns, name);
                                }
                            };
                        }
                        return super.child(ns, name);
                    }
                };
            }
        });


        log.info("findFirstClass:" + className[0]);
        return TypeDescriptionHelper.convertByFullClassName(className[0]);
    }

}
