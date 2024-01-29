#ifndef PQPIXMAP_H
#define PQPIXMAP_H

/**
 * @brief pQPixmap a smart pointer to a QPixmap to avoid much data on the stack
 * The smart pointer always has an object (!)
 */
class pQPixmap : public std::shared_ptr<QPixmap>
{
public:
    /**
     * @brief pQPixmap enforce a creation of an empty QPixmap
     */
    pQPixmap()
        : std::shared_ptr<QPixmap>(new QPixmap())
    {
    }

    /**
     * @brief pPixmap load from file
     * @param filename the file
     */
    pQPixmap(const QString& filename)
        : std::shared_ptr<QPixmap>(new QPixmap(filename))
    {}

    /**
     * @brief pQPixmap create this as reference to another pQPixmap
     * @param other the other smart pointer to the QPixmap
     */
    pQPixmap(const pQPixmap& other)
        : std::shared_ptr<QPixmap>()
    {
        this->reset();
        std::shared_ptr<QPixmap>::operator =( other);
        if (other.get() == 0) {
            std::shared_ptr<QPixmap>::operator =( std::make_shared<QPixmap>(QPixmap()));
        }
    }

    /**
     * @brief pQPixmap obtain ownership of a QPixmap pointer
     * @param other the pointer to an QPixmap -> must not be deleted somewhere else
     */
    pQPixmap(QPixmap* other)
        : std::shared_ptr<QPixmap>(other)
    {
    }

    /**
     * @brief pQPixmap a smart pointer to an image given an actual QPixmap object
     * @param image the object to be referenced to
     */
    pQPixmap(const QPixmap& image)
        : std::shared_ptr<QPixmap>(new QPixmap(image))
    {
    }
};

#endif // PQPIXMAP_H
