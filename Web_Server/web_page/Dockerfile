FROM python:3.6

RUN apt-get update

COPY requirements.txt /tmp
RUN pip install -r /tmp/requirements.txt
COPY . /app
WORKDIR /app
EXPOSE 6000
CMD ["python", "main.py"]