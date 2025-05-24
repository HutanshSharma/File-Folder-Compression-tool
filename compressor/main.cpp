#include <QCoreApplication>
#include <QIODevice>
#include <QFile>
#include <QDir>
#include <QMimeDatabase>
#include <iostream>
using namespace std;

bool compresscheck(const QString &path){
    QMimeDatabase db;
    QMimeType type=db.mimeTypeForFile(path);

    if(type.name().startsWith("image/") || type.name().contains("zip") || type.name().contains("compressed") || type.name().contains("video/")) {
        qInfo()<<"Skipping already compressed file:"<<path;
        return false;
    }
    return true;
}

bool compressfile(const QString &pathorg, const QString &pathcompressed) {
    QFile org(pathorg);
    QFile comp(pathcompressed);

    if(!compresscheck(pathorg)) return false;

    if(!org.open(QIODevice::ReadOnly)){
        qInfo()<<"couldn't load the original file";
        return false;
    }

    if(!comp.open(QIODevice::WriteOnly)) {
        qInfo()<<"couldn't form the compressed file";
        return false;
    }

    const QByteArray header("~[a]~");
    comp.write(header);

    QFileInfo orginfo(pathorg);
    QString filename=orginfo.fileName();
    QByteArray metadata;
    metadata.append(filename.toUtf8());
    qint64 metasize=metadata.size();
    if(comp.write(reinterpret_cast<const char*>(&metasize),sizeof(qint64))!=sizeof(qint64)){
        qInfo()<<"could not write the metadatasize";
        return false;
    }
    if(comp.write(metadata)!=metasize){
        qInfo()<<"could not write the metadata";
        return false;
    }

    const int chunksize = 1024;

    while(!org.atEnd()) {
        QByteArray buffer = org.read(chunksize);
        QByteArray compressed = qCompress(buffer, 9);
        const qint64 size = compressed.size();

        if(comp.write(reinterpret_cast<const char*>(&size),sizeof(qint64))!=sizeof(qint64)){
            qInfo()<<"could not write the size of the compressed chunk";
            return false;
        }
        if(comp.write(compressed)!=size){
            qInfo()<<"could not write the compressed data";
            return false;
        }
    }

    qint64 orgfilesize=org.size();
    qint64 compfilesize=comp.size();

    org.close();
    comp.close();

    if(orgfilesize<=compfilesize){
        QFile::remove(pathcompressed);
        qInfo()<<"Already compressed to the maximum -"<<pathorg;
        return false;
    }
    return true;
}

bool decompressfile(const QString &pathcompressed, const QString &pathdecomp) {
    QFile comp(pathcompressed);
    QFile decomp(pathdecomp);

    if(!comp.open(QIODevice::ReadOnly)){
        qInfo()<<"couldn't load the compressed file";
        return false;
    }
    if(!decomp.open(QIODevice::WriteOnly | QIODevice::Truncate)){
        qInfo()<<"couldn't form the decompressed file";
        return false;
    }

    const QByteArray myHeader("~[a]~");
    QByteArray header=comp.read(myHeader.size());
    if(header!=myHeader){
        qInfo()<<"Wrong header";
        return false;
    }

    qint64 metasize;
    if(comp.read(reinterpret_cast<char*>(&metasize),sizeof(qint64))!=sizeof(qint64)){
        qInfo()<<"could not read the metadatasize";
        return false;
    }
    if(metasize<=0){
        qInfo()<<"No such file";
        return false;
    }

    QByteArray filename=comp.read(metasize);
    if (filename.size() != metasize) {
        qInfo() << "Failed to read full metadata";
        return false;
    }

    while(!comp.atEnd()) {
        qint64 size;
        if(comp.read(reinterpret_cast<char*>(&size),sizeof(qint64))!=sizeof(qint64)){
            qInfo()<<"could not read the size of the compressed chunk";
            return false;
        }

        if(size<=0){
            qInfo()<<"The size of the compressed chunk could not be read correctly";
            return false;
        }

        QByteArray buffer=comp.read(size);

        if(buffer.size()!=size){
            qInfo()<<"Could not read the compressed chunk correctly";
            return false;
        }

        QByteArray decompressed = qUncompress(buffer);
        if(decompressed.isEmpty()) {
            qDebug()<<"Decompression failed";
            return false;
        }

        if(decomp.write(decompressed)!=decompressed.size()){
            qInfo()<<"could not write the decompressed chunk";
            return false;
        }
    }

    comp.close();
    decomp.close();

    return true;
}

bool copyfolder(const QString &srcpath,const QString &copypath){
    QDir src(srcpath);
    QDir copy(copypath);

    if(!src.exists()) return false;

    if(!copy.mkpath(".")){
        qInfo()<<"could not create copy path";
        return false;
    };

    foreach(auto i,src.entryList(QDir::Files)){
        if(!QFile::copy(src.filePath(i),copy.filePath(i))) qInfo()<<"Failed to cpy file";
    }

    foreach(auto i,src.entryList(QDir::Dirs | QDir::NoDotAndDotDot)){
        if(!copyfolder(src.filePath(i),copy.filePath(i))) return false;
    }
    return true;
}

void compressingfiles(QDir &root){
    foreach(auto i,root.entryList(QDir::Files)){

        const QString path=root.filePath(i);
        QFileInfo info(path);
        const QString base=info.absolutePath() + "/" + info.completeBaseName();
        const QString pathcompress=base + "-comp.txt";

        if(compressfile(path,pathcompress)){
            if(!QFile::remove(path)) qInfo()<<"Original file still in place";
        }
    }

    foreach(auto  i,root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)){
        if(i.isDir()){
            QDir child(i.filePath());
            qInfo()<<"Compressing.."<<i.absoluteFilePath();
            compressingfiles(child);
        }
    }
}

void decompressingfiles(QDir &root){
    foreach(auto i,root.entryList(QDir::Files)){

        const QString path=root.filePath(i);
        QFile comp(path);
        QFileInfo info(path);

        if(!comp.open(QIODevice::ReadOnly)){
            qInfo()<<"Could not open file for decompressing";
            continue;
        }

        QString filebasename=info.completeBaseName();
        if(!filebasename.endsWith("comp") || filebasename.endsWith("decomp")){
            qInfo()<<"Not a compressed file";
            continue;
        }

        const QByteArray myHeader("~[a]~");
        comp.seek(myHeader.size());

        qint64 metasize;
        if(comp.read(reinterpret_cast<char*>(&metasize),sizeof(qint64))!=sizeof(qint64)){
            qInfo()<<"could not read the metadatasize";
            continue;
        }

        QByteArray filename=comp.read(metasize);
        if(filename.size()!=metasize){
            qInfo()<<"could not read the metadata";
            continue;
        }
        QString pathdecompress = info.absolutePath() + "/" + QString::fromUtf8(filename);
        pathdecompress.insert(pathdecompress.lastIndexOf('.'), "-decomp");

        if(decompressfile(path,pathdecompress)){
            if(!comp.remove()) qInfo()<<"Original file still in place";
        }
    }

    foreach(auto  i,root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)){
        if(i.isDir()){
            QDir child(i.filePath());
            qInfo()<<"Decompressing.."<<i.absolutePath();
            decompressingfiles(child);
        }
    }
}

bool compress_and_decompress_folder(const QString &root){
    QDir src(root);

    if(!src.exists()){
        qInfo()<<"source directory doesn't exist";
        return false;
    }

    QString copy=src.absolutePath()+"-compressed";

    if(!copyfolder(root,copy)){
        qInfo()<<"Failed to copy folder";
        return false;
    }

    QDir dir(copy);
    qInfo()<<"Compressing.."<<dir.absolutePath();
    compressingfiles(dir);

    QString copydecom=src.absolutePath()+"-decompressed";
    if(!copyfolder(copy,copydecom)){
        qInfo()<<"could not make a copy for decompressing";
        return false;
    }

    QDir dirdecom(copydecom);
    qInfo()<<"Decompressing.."<<dirdecom.absolutePath();
    decompressingfiles(dirdecom);
    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    int choice;
    qInfo()<<"Do you want to compress a folder or a file:";
    qInfo()<<"1. File";
    qInfo()<<"2. Folder";
    cin>>choice;
    qInfo()<<"Enter the absolute path (with '/' and no double quotes) :";
    QTextStream qtin(stdin);
    QString path=qtin.readLine().trimmed();

    switch(choice){
    case 1:{
        QFileInfo info(path);
        QString compresspath=info.absolutePath()+'/'+info.completeBaseName()+"-comp.txt";
        if(compressfile(path,compresspath)){
            QString decompresspath=info.absolutePath()+'/'+info.completeBaseName()+"-decomp."+info.suffix();
            qInfo()<<"Compressing complete";
            if(decompressfile(compresspath,decompresspath)){
                qInfo()<<"Decompressing complete";
            }
            else qInfo()<<"Decompressing failed";
        }
        else qInfo()<<"Compressing failed";
        break;
    }

    case 2:{
        if(compress_and_decompress_folder(path)) qInfo()<<"The folder compressed and decompressed successfully";
        else qInfo()<<"Process failed";
        break;
    }

    default:{
        qInfo()<<"Invalid choice";
        break;
    }

    }

    return a.exec();
}
