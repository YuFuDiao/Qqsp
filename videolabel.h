#ifndef VIDEOLABEL_H
#define VIDEOLABEL_H

#include <QMediaPlayer>
#include <QSize>
#include <QVideoFrame>
#include <QVideoSink>
#include <QVideoWidget>

class VideoLabel: public QVideoWidget {
signals:
	void medialLoaded();

public:
	explicit VideoLabel(QString path, QString filename, QWidget* parent = 0);
	~VideoLabel();
	bool videoError();
	QSize getResolution();

	bool hasFrame() { return m_medialLoaded; }

	bool resolution_set;

private:
	QString m_path;
	QString m_filename;
	QMediaPlayer mediaPlayer;
	bool m_videoError;
	bool m_medialLoaded;
	QSize m_resolution;
};

#endif // VIDEOLABEL_H
