FROM debian:12-slim
RUN apt update && apt upgrade -y
SHELL [ "/bin/bash", "-c" ]
ENV SHELL=/bin/bash
WORKDIR /app
COPY /build/api-cpp-crow-exe .
RUN chmod +x /app/api-cpp-crow-exe
CMD /app/api-cpp-crow-exe
