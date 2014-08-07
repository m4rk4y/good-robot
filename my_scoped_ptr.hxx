#ifndef MY_SCOPED_PTR_HXX
#define MY_SCOPED_PTR_HXX

// A very minimal implementation based on the Boost header, only useful for
// new-ed objects (and not even new[] ones).

namespace scoping
{

template < class T > class scoped_ptr
{
    public:
        explicit scoped_ptr ( T * obj = 0 )
          : m_obj ( obj ) {}
        ~scoped_ptr()
        {
            delete m_obj;
        }
        scoped_ptr & operator = ( T * ptr )
        {
            reset ( ptr );
            return *this;
        }
        void reset ( T * ptr = 0 )
        {
            delete m_obj;
            m_obj = ptr;
        }
        T & operator * () const
        {
            return *m_obj;
        }
        T * operator -> () const
        {
            return m_obj;
        }
        T * get() const
        {
            return m_obj;
        }

    private:
        T * m_obj;
};

}   // end namespace scoping

#endif  // MY_SCOPED_PTR_HXX